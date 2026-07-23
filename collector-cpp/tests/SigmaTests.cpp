#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "DetectionEngine.h"
#include "Event.h"
#include "Logger.h"
#include "Rule.h"
#include "RuleLoader.h"
#include "SigmaLoader.h"
#include "SigmaParser.h"
#include "SigmaTranslator.h"

namespace sentinelforge {
namespace {

constexpr const char* kValidSigmaYaml = R"(
title: Suspicious PowerShell
id: 8d2b1234
status: experimental
description: Detect encoded PowerShell
author: ignored-author
falsepositives:
  - ignored
logsource:
  category: process_creation
detection:
  selection:
    process_name: powershell.exe
    command_line|contains: "-enc"
    Image|endswith: ignored.exe
  condition: selection
level: high
tags:
  - attack.execution
  - attack.t1059.001
)";

class SigmaTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        tempDir_ = std::filesystem::temp_directory_path() /
                   ("sf_sigma_" + std::string(info->name()));
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
        std::filesystem::create_directories(tempDir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
    }

    Logger quietLogger_{LoggingSettings{LogLevel::Error, true, false, {}}};
    SigmaParser parser_;
    SigmaTranslator translator_;
    std::filesystem::path tempDir_;
};

TEST_F(SigmaTest, ValidSigmaRuleParses) {
    const SigmaParseResult result = parser_.ParseString(kValidSigmaYaml, "valid.yml");

    ASSERT_TRUE(result.IsValid()) << "A Phase-1 Sigma rule must parse successfully";
    EXPECT_EQ(result.Rule().Title(), "Suspicious PowerShell");
    EXPECT_EQ(result.Rule().Id(), "8d2b1234");
    EXPECT_EQ(result.Rule().Status(), "experimental");
    EXPECT_EQ(result.Rule().Description(), "Detect encoded PowerShell");
    EXPECT_EQ(result.Rule().LogsourceCategory(), "process_creation");
    EXPECT_EQ(result.Rule().ProcessName(), "powershell.exe");
    EXPECT_EQ(result.Rule().CommandLineContains(), "-enc");
    EXPECT_EQ(result.Rule().Condition(), "selection");
    EXPECT_EQ(result.Rule().Level(), "high");
    ASSERT_EQ(result.Rule().Tags().size(), 2u);
}

TEST_F(SigmaTest, MissingTitleRejected) {
    const std::string yaml = R"(
detection:
  selection:
    process_name: powershell.exe
    command_line|contains: "-enc"
  condition: selection
level: high
)";

    const SigmaParseResult result = parser_.ParseString(yaml, "missing-title.yml");

    EXPECT_FALSE(result.IsValid());
    ASSERT_FALSE(result.Errors().empty());
    EXPECT_NE(result.Errors().front().find("title"), std::string::npos);
}

TEST_F(SigmaTest, MissingDetectionRejected) {
    const std::string yaml = R"(
title: No Detection Block
level: high
)";

    const SigmaParseResult result = parser_.ParseString(yaml, "missing-detection.yml");

    EXPECT_FALSE(result.IsValid());
    bool found = false;
    for (const auto& error : result.Errors()) {
        if (error.find("detection") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Rejection must mention missing detection";
}

TEST_F(SigmaTest, InvalidRuleRejected) {
    const std::string yaml = R"(
title: Bad Condition
detection:
  selection:
    process_name: cmd.exe
  condition: selection and filter
level: high
tags:
  - attack.t1059.001
)";

    const SigmaParseResult result = parser_.ParseString(yaml, "bad-condition.yml");

    EXPECT_FALSE(result.IsValid());
    bool found = false;
    for (const auto& error : result.Errors()) {
        if (error.find("condition") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Unsupported conditions must be rejected";
}

TEST_F(SigmaTest, TranslationProducesExpectedRule) {
    const SigmaParseResult parsed = parser_.ParseString(kValidSigmaYaml, "valid.yml");
    ASSERT_TRUE(parsed.IsValid());

    const Rule rule = translator_.Translate(parsed.Rule());

    EXPECT_EQ(rule.RuleName(), "Suspicious PowerShell");
    EXPECT_EQ(rule.ProcessName(), "powershell.exe");
    EXPECT_EQ(rule.CommandLineContains(), "-enc");
    EXPECT_EQ(rule.Severity(), "High");
    EXPECT_EQ(rule.Mitre(), "T1059.001");
    EXPECT_EQ(rule.Description(), "Detect encoded PowerShell");
}

TEST_F(SigmaTest, UnsupportedFieldsIgnoredSafely) {
    const SigmaParseResult result = parser_.ParseString(kValidSigmaYaml, "valid.yml");
    ASSERT_TRUE(result.IsValid());
    // author, falsepositives, and Image|endswith are present in the YAML but
    // must not prevent a successful parse of the supported subset.
    EXPECT_EQ(result.Rule().ProcessName(), "powershell.exe");
    EXPECT_EQ(result.Rule().CommandLineContains(), "-enc");
}

TEST_F(SigmaTest, MixedSigmaAndNativeRulesLoadTogether) {
    const std::filesystem::path nativeDir = tempDir_ / "native";
    const std::filesystem::path sigmaDir = tempDir_ / "sigma";
    std::filesystem::create_directories(nativeDir);
    std::filesystem::create_directories(sigmaDir);

    {
        std::ofstream out(nativeDir / "whoami.json");
        out << R"({
  "rule_name": "Cmd Whoami",
  "process_name": "cmd.exe",
  "command_line_contains": "whoami",
  "severity": "Medium",
  "mitre": "T1033"
})";
    }
    {
        std::ofstream sigmaOut(sigmaDir / "encoded.yml");
        sigmaOut << kValidSigmaYaml;
    }

    RuleLoader nativeLoader;
    SigmaLoader sigmaLoader;

    const RuleLoadResult native = nativeLoader.LoadDirectory(nativeDir);
    ASSERT_EQ(native.Accepted().size(), 1u);

    std::unordered_set<std::string> names;
    names.insert(native.Accepted().front().RuleName());

    const RuleLoadResult sigma = sigmaLoader.LoadDirectory(sigmaDir, names, quietLogger_);
    ASSERT_EQ(sigma.Accepted().size(), 1u) << "Sigma rule should be accepted beside native rules";

    std::vector<Rule> combined = native.Accepted();
    combined.push_back(sigma.Accepted().front());
    EXPECT_EQ(combined.size(), 2u);

    const Event event("2026-07-20T14:32:07Z", "HOST", "user", "powershell.exe", "explorer.exe",
                      "powershell.exe -enc ZQBjAGgAbwA=", 1000);
    DetectionEngine engine;
    const auto results = engine.Evaluate(event, combined);

    std::size_t matches = 0;
    for (const auto& result : results) {
        if (result.Matched()) {
            ++matches;
        }
    }
    EXPECT_EQ(matches, 1u) << "Translated Sigma rule must match via DetectionEngine";
}

TEST_F(SigmaTest, TranslatedRuleMatchesLikeNativeEquivalent) {
    const SigmaParseResult parsed = parser_.ParseString(kValidSigmaYaml, "valid.yml");
    ASSERT_TRUE(parsed.IsValid());
    const Rule fromSigma = translator_.Translate(parsed.Rule());

    const Rule native("Suspicious PowerShell", "powershell.exe", "-enc", "High", "T1059.001", "",
                      "", "Detect encoded PowerShell", "");

    const Event event("2026-07-20T14:32:07Z", "HOST", "user", "powershell.exe", "explorer.exe",
                      "powershell.exe -enc ZQBjAGgAbwA=", 1000);

    DetectionEngine engine;
    const auto sigmaResults = engine.Evaluate(event, {fromSigma});
    const auto nativeResults = engine.Evaluate(event, {native});

    ASSERT_EQ(sigmaResults.size(), 1u);
    ASSERT_EQ(nativeResults.size(), 1u);
    EXPECT_EQ(sigmaResults.front().Matched(), nativeResults.front().Matched());
    EXPECT_EQ(sigmaResults.front().Reason(), nativeResults.front().Reason());
}

}  // namespace
}  // namespace sentinelforge
