#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "DetectionReport.h"
#include "DetectionResult.h"
#include "JsonExporter.h"
#include "Logger.h"
#include "NormalizedEvent.h"

namespace sentinelforge {
namespace {

NormalizedEvent MakeNormalized(const std::string& processName = "powershell.exe") {
    return NormalizedEvent("2026-07-20T14:32:07Z", "WORKSTATION-07", "CORP\\jsmith", "", processName,
                           "explorer.exe", "powershell.exe -enc SGVsbG8=", "", "", "", "", "", "",
                           "json");
}

DetectionResult MakeMatch(const std::string& ruleName, const std::string& reason) {
    return DetectionResult(true, ruleName, "High", "T1059.001", reason);
}

DetectionResult MakeMiss(const std::string& ruleName) {
    return DetectionResult(false, ruleName, "Low", "T1059.001", "no match");
}

DetectionReport MakeReport(std::vector<DetectionResult> results) {
    const NormalizedEvent event = MakeNormalized();
    const std::size_t evaluated = results.size();
    return DetectionReport(event, evaluated, evaluated, std::move(results));
}

class JsonExporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        tempDir_ = std::filesystem::temp_directory_path() /
                   ("sf_json_export_" + std::string(info->name()));
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
        std::filesystem::create_directories(tempDir_);
        outputPath_ = tempDir_ / "detections.json";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
    }

    JsonExporter exporter_;
    Logger quietLogger_{LoggingSettings{LogLevel::Error, true, false, {}}};
    std::filesystem::path tempDir_;
    std::filesystem::path outputPath_;
};

TEST_F(JsonExporterTest, ValidJsonGenerated) {
    const DetectionReport report = MakeReport({MakeMatch(
        "Suspicious PowerShell",
        "process_name matched 'powershell.exe' and command_line contains '-enc'")});

    const nlohmann::json document = exporter_.BuildDocument(report);

    EXPECT_TRUE(document.contains("generated_at"));
    EXPECT_TRUE(document.at("generated_at").is_string());
    EXPECT_FALSE(document.at("generated_at").get<std::string>().empty());
    EXPECT_EQ(document.at("rules_loaded").get<std::size_t>(), 1u);
    EXPECT_EQ(document.at("rules_evaluated").get<std::size_t>(), 1u);
    EXPECT_EQ(document.at("matches").get<std::size_t>(), 1u);
    ASSERT_TRUE(document.at("detections").is_array());
    ASSERT_EQ(document.at("detections").size(), 1u);
    ASSERT_TRUE(document.contains("correlation_alerts"));
    ASSERT_TRUE(document.at("correlation_alerts").is_array());
    EXPECT_TRUE(document.at("correlation_alerts").empty());

    const auto& detection = document.at("detections").at(0);
    EXPECT_EQ(detection.at("rule"), "Suspicious PowerShell");
    EXPECT_EQ(detection.at("severity"), "High");
    EXPECT_EQ(detection.at("mitre"), "T1059.001");
    EXPECT_EQ(detection.at("process"), "powershell.exe");
    EXPECT_NE(detection.at("reason").get<std::string>().find("-enc"), std::string::npos);

    // Round-trip through dump/parse to prove the document is valid JSON text.
    EXPECT_NO_THROW({
        const nlohmann::json parsed = nlohmann::json::parse(document.dump());
        EXPECT_EQ(parsed.at("matches").get<std::size_t>(), 1u);
    });
}

TEST_F(JsonExporterTest, EmptyDetectionsExportedCorrectly) {
    const DetectionReport report = MakeReport({MakeMiss("Cmd Whoami"), MakeMiss("Mimikatz")});

    const nlohmann::json document = exporter_.BuildDocument(report);

    EXPECT_EQ(document.at("matches").get<std::size_t>(), 0u);
    ASSERT_TRUE(document.at("detections").is_array());
    EXPECT_TRUE(document.at("detections").empty())
        << "Zero matches must still produce an empty detections array";
    ASSERT_TRUE(document.contains("correlation_alerts"));
    EXPECT_TRUE(document.at("correlation_alerts").empty());
}

TEST_F(JsonExporterTest, MultipleDetectionsExported) {
    const DetectionReport report = MakeReport(
        {MakeMatch("Suspicious PowerShell", "reason-a"), MakeMiss("Cmd Whoami"),
         MakeMatch("Encoded Command", "reason-b")});

    const nlohmann::json document = exporter_.BuildDocument(report);

    EXPECT_EQ(document.at("matches").get<std::size_t>(), 2u);
    ASSERT_EQ(document.at("detections").size(), 2u);
    EXPECT_EQ(document.at("detections").at(0).at("rule"), "Suspicious PowerShell");
    EXPECT_EQ(document.at("detections").at(1).at("rule"), "Encoded Command");
}

TEST_F(JsonExporterTest, DisabledExportProducesNoFile) {
    const DetectionReport report = MakeReport({MakeMatch("Suspicious PowerShell", "reason")});

    JsonExportSettings settings;
    settings.enabled = false;
    settings.outputFile = outputPath_;

    EXPECT_TRUE(exporter_.Export(report, settings, quietLogger_));
    EXPECT_FALSE(std::filesystem::exists(outputPath_))
        << "A disabled export must not create an output file";
}

TEST_F(JsonExporterTest, InvalidOutputPathHandledSafely) {
    const DetectionReport report = MakeReport({MakeMatch("Suspicious PowerShell", "reason")});

    // Parent path is a regular file, so create_directories / open must fail.
    const std::filesystem::path blocker = tempDir_ / "not-a-directory";
    {
        std::ofstream out(blocker);
        out << "blocker";
    }
    const std::filesystem::path badPath = blocker / "detections.json";

    JsonExportSettings settings;
    settings.enabled = true;
    settings.outputFile = badPath;

    EXPECT_FALSE(exporter_.Export(report, settings, quietLogger_))
        << "An unwritable output path must fail without throwing";
    EXPECT_FALSE(std::filesystem::exists(badPath));
}

TEST_F(JsonExporterTest, ExportWritesFile) {
    const DetectionReport report = MakeReport({MakeMatch(
        "Suspicious PowerShell",
        "process_name matched 'powershell.exe' and command_line contains '-enc'")});

    JsonExportSettings settings;
    settings.enabled = true;
    settings.outputFile = outputPath_;

    ASSERT_TRUE(exporter_.Export(report, settings, quietLogger_));
    ASSERT_TRUE(std::filesystem::exists(outputPath_));

    std::ifstream in(outputPath_);
    nlohmann::json parsed;
    ASSERT_NO_THROW(in >> parsed);
    EXPECT_EQ(parsed.at("matches").get<std::size_t>(), 1u);
    EXPECT_EQ(parsed.at("detections").at(0).at("rule"), "Suspicious PowerShell");
}

}  // namespace
}  // namespace sentinelforge
