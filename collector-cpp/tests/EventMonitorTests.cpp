#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "Configuration.h"
#include "DetectionEngine.h"
#include "EventMonitor.h"
#include "EventParser.h"
#include "JsonExporter.h"
#include "Logger.h"
#include "PerformanceProfiler.h"
#include "ReportPrinter.h"
#include "Rule.h"

namespace sentinelforge {
namespace {

constexpr const char* kValidEventJson = R"({
  "timestamp": "2026-07-20T14:32:07Z",
  "hostname": "WORKSTATION-07",
  "username": "CORP\\jsmith",
  "process_name": "powershell.exe",
  "parent_process": "explorer.exe",
  "command_line": "powershell.exe -enc SGVsbG8=",
  "pid": 4821
})";

class EventMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        tempDir_ = std::filesystem::temp_directory_path() /
                   ("sf_monitor_" + std::string(info->name()));
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
        incoming_ = tempDir_ / "incoming";
        processed_ = tempDir_ / "processed";
        failed_ = tempDir_ / "failed";
        exportPath_ = tempDir_ / "detections.json";
        std::filesystem::create_directories(incoming_);
        std::filesystem::create_directories(processed_);
        std::filesystem::create_directories(failed_);

        rules_ = {Rule("Suspicious PowerShell", "powershell.exe", "-enc", "High", "T1059.001", "",
                       "", "", "")};

        settings_.enabled = true;
        settings_.inputDirectory = incoming_;
        settings_.processedDirectory = processed_;
        settings_.failedDirectory = failed_;
        settings_.pollIntervalMs = 50;

        jsonExport_.enabled = true;
        jsonExport_.outputFile = exportPath_;
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
    }

    EventMonitor MakeMonitor() {
        return EventMonitor(settings_, jsonExport_, rules_, eventParser_, detectionEngine_,
                            reportPrinter_, jsonExporter_, profiler_, quietLogger_);
    }

    void WriteIncoming(const std::string& name, const std::string& contents) {
        std::ofstream out(incoming_ / name);
        out << contents;
    }

    bool WaitUntil(const std::function<bool()>& predicate,
                   std::chrono::milliseconds timeout = std::chrono::milliseconds{2000}) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{20});
        }
        return predicate();
    }

    std::size_t CountJson(const std::filesystem::path& directory) const {
        std::size_t count = 0;
        if (!std::filesystem::is_directory(directory)) {
            return 0;
        }
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                ++count;
            }
        }
        return count;
    }

    Logger quietLogger_{LoggingSettings{LogLevel::Error, true, false, {}}};
    EventParser eventParser_;
    DetectionEngine detectionEngine_;
    ReportPrinter reportPrinter_;
    JsonExporter jsonExporter_;
    PerformanceProfiler profiler_;
    MonitoringSettings settings_;
    JsonExportSettings jsonExport_;
    std::vector<Rule> rules_;
    std::filesystem::path tempDir_;
    std::filesystem::path incoming_;
    std::filesystem::path processed_;
    std::filesystem::path failed_;
    std::filesystem::path exportPath_;
};

TEST_F(EventMonitorTest, StartupAndGracefulShutdown) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { EXPECT_EQ(monitor.Run(), 0); });

    ASSERT_TRUE(WaitUntil([&monitor]() { return true; }, std::chrono::milliseconds{100}));
    monitor.RequestStop();
    worker.join();

    EXPECT_TRUE(monitor.StopRequested());
    EXPECT_EQ(CountJson(processed_), 0u);
}

TEST_F(EventMonitorTest, EmptyDirectoryProducesNoOutput) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds{150});
    monitor.RequestStop();
    worker.join();

    EXPECT_EQ(CountJson(incoming_), 0u);
    EXPECT_EQ(CountJson(processed_), 0u);
    EXPECT_FALSE(std::filesystem::exists(exportPath_));
}

TEST_F(EventMonitorTest, SingleEventProcessedAndArchived) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("event1.json", kValidEventJson);

    ASSERT_TRUE(WaitUntil([this]() { return CountJson(processed_) == 1; }));
    monitor.RequestStop();
    worker.join();

    EXPECT_EQ(CountJson(incoming_), 0u);
    EXPECT_EQ(CountJson(processed_), 1u);
    EXPECT_TRUE(std::filesystem::exists(processed_ / "event1.json"));
    EXPECT_TRUE(std::filesystem::exists(exportPath_));
}

TEST_F(EventMonitorTest, MultipleEventsProcessed) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("a.json", kValidEventJson);
    WriteIncoming("b.json", kValidEventJson);

    ASSERT_TRUE(WaitUntil([this]() { return CountJson(processed_) == 2; }));
    monitor.RequestStop();
    worker.join();

    EXPECT_EQ(CountJson(incoming_), 0u);
    EXPECT_EQ(CountJson(processed_), 2u);
}

TEST_F(EventMonitorTest, MalformedJsonMovedToFailed) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("bad.json", "{ not-valid-json ");

    ASSERT_TRUE(WaitUntil([this]() { return CountJson(failed_) == 1; }));
    monitor.RequestStop();
    worker.join();

    EXPECT_EQ(CountJson(incoming_), 0u);
    EXPECT_EQ(CountJson(processed_), 0u);
    EXPECT_TRUE(std::filesystem::exists(failed_ / "bad.json"));
    EXPECT_FALSE(std::filesystem::exists(exportPath_));
}

TEST_F(EventMonitorTest, ValidJsonMovedToProcessed) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("good.json", kValidEventJson);

    ASSERT_TRUE(WaitUntil([this]() { return CountJson(processed_) == 1; }));
    monitor.RequestStop();
    worker.join();

    EXPECT_EQ(CountJson(incoming_), 0u);
    EXPECT_EQ(CountJson(failed_), 0u);
    EXPECT_TRUE(std::filesystem::exists(processed_ / "good.json"));
    EXPECT_TRUE(std::filesystem::exists(exportPath_));
}

TEST_F(EventMonitorTest, MonitoringContinuesAfterFailure) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("01_bad.json", "{ broken ");
    WriteIncoming("02_good.json", kValidEventJson);

    ASSERT_TRUE(WaitUntil(
        [this]() { return CountJson(failed_) == 1 && CountJson(processed_) == 1; }));
    monitor.RequestStop();
    worker.join();

    EXPECT_EQ(CountJson(incoming_), 0u);
    EXPECT_TRUE(std::filesystem::exists(failed_ / "01_bad.json"));
    EXPECT_TRUE(std::filesystem::exists(processed_ / "02_good.json"));
    EXPECT_TRUE(std::filesystem::exists(exportPath_));
}

TEST_F(EventMonitorTest, FileMovementPreservesContents) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("keep.json", kValidEventJson);

    ASSERT_TRUE(WaitUntil([this]() { return std::filesystem::exists(processed_ / "keep.json"); }));
    monitor.RequestStop();
    worker.join();

    std::ifstream in(processed_ / "keep.json");
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(contents.find("powershell.exe"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(incoming_ / "keep.json"));
}

TEST_F(EventMonitorTest, RequestStopFinishesCurrentEvent) {
    EventMonitor monitor = MakeMonitor();
    std::thread worker([&monitor]() { monitor.Run(); });

    WriteIncoming("one.json", kValidEventJson);
    ASSERT_TRUE(WaitUntil([this]() { return CountJson(processed_) == 1; }));

    monitor.RequestStop();
    worker.join();

    EXPECT_TRUE(monitor.StopRequested());
    EXPECT_EQ(CountJson(processed_), 1u);
}

TEST_F(EventMonitorTest, ConfigurationLoading) {
    // Path layout: <repo>/config/file.json → base is parent of config dir.
    const std::filesystem::path configRoot = tempDir_ / "config";
    std::filesystem::create_directories(configRoot);
    const std::filesystem::path configPath = configRoot / "sentinelforge.json";

    const std::filesystem::path input = tempDir_ / "in";
    const std::filesystem::path processed = tempDir_ / "out";
    const std::filesystem::path failed = tempDir_ / "fail";

    std::ofstream out(configPath);
    out << "{\n"
        << "  \"monitoring\": {\n"
        << "    \"enabled\": true,\n"
        << "    \"input_directory\": \"" << input.generic_string() << "\",\n"
        << "    \"processed_directory\": \"" << processed.generic_string() << "\",\n"
        << "    \"failed_directory\": \"" << failed.generic_string() << "\",\n"
        << "    \"poll_interval_ms\": 250\n"
        << "  }\n"
        << "}\n";
    out.close();

    const Configuration config = Configuration::LoadFromFile(configPath, quietLogger_);
    EXPECT_TRUE(config.Monitoring().enabled);
    EXPECT_EQ(config.Monitoring().inputDirectory, input);
    EXPECT_EQ(config.Monitoring().processedDirectory, processed);
    EXPECT_EQ(config.Monitoring().failedDirectory, failed);
    EXPECT_EQ(config.Monitoring().pollIntervalMs, 250u);
}

}  // namespace
}  // namespace sentinelforge
