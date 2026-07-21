#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "DetectionEngine.h"
#include "DetectionResult.h"
#include "Event.h"
#include "Rule.h"

namespace sentinelforge {
namespace {

// Synthetic telemetry event. No JSON, no files: the domain object is built
// directly so detection logic is tested in isolation.
Event MakeEvent(const std::string& processName, const std::string& commandLine) {
    return Event("2024-01-01T00:00:00Z", "WORKSTATION", "analyst", processName, "explorer.exe",
                 commandLine, static_cast<std::uint32_t>(4242));
}

// Synthetic rule carrying only the fields the engine matches on, plus fixed
// metadata so the resulting DetectionResult is fully populated.
Rule MakeRule(const std::string& name,
              const std::string& processName,
              const std::string& commandLineContains) {
    return Rule(name, processName, commandLineContains, "High", "T1059.001", "SentinelForge", "1.0",
                "test rule", "2024-01-01");
}

std::size_t CountMatches(const std::vector<DetectionResult>& results) {
    return static_cast<std::size_t>(
        std::count_if(results.begin(), results.end(),
                      [](const DetectionResult& result) { return result.Matched(); }));
}

class DetectionEngineTest : public ::testing::Test {
protected:
    DetectionEngine engine_;
};

TEST_F(DetectionEngineTest, MatchingRuleProducesOneDetection) {
    const Event event = MakeEvent("powershell.exe", "powershell.exe -enc ZQBjAGgAbwA=");
    const std::vector<Rule> rules = {MakeRule("Encoded PowerShell", "powershell.exe", "-enc")};

    const std::vector<DetectionResult> results = engine_.Evaluate(event, rules);

    ASSERT_EQ(results.size(), 1u) << "One rule should yield one evaluation result";
    EXPECT_EQ(CountMatches(results), 1u) << "The matching rule should produce exactly one detection";
    EXPECT_TRUE(results.front().Matched()) << "The single result should be a match";
}

TEST_F(DetectionEngineTest, NonMatchingRuleProducesZeroDetections) {
    const Event event = MakeEvent("notepad.exe", "notepad.exe readme.txt");
    const std::vector<Rule> rules = {MakeRule("Encoded PowerShell", "powershell.exe", "-enc")};

    const std::vector<DetectionResult> results = engine_.Evaluate(event, rules);

    ASSERT_EQ(results.size(), 1u) << "One rule should still yield one evaluation result";
    EXPECT_EQ(CountMatches(results), 0u) << "A rule that does not apply should produce no detection";
    EXPECT_FALSE(results.front().Matched()) << "The single result should not be a match";
}

TEST_F(DetectionEngineTest, MultipleRulesReturnCorrectMatchCount) {
    const Event event = MakeEvent("powershell.exe", "powershell.exe -enc ZQBjAGgAbwA=");
    const std::vector<Rule> rules = {
        MakeRule("Encoded PowerShell", "powershell.exe", "-enc"),   // matches
        MakeRule("Any PowerShell", "powershell.exe", "powershell"), // matches (case-insensitive)
        MakeRule("Cmd Whoami", "cmd.exe", "whoami"),                // process mismatch
    };

    const std::vector<DetectionResult> results = engine_.Evaluate(event, rules);

    ASSERT_EQ(results.size(), 3u) << "Every rule should be evaluated exactly once";
    EXPECT_EQ(CountMatches(results), 2u) << "Two of the three rules should match this event";
}

TEST_F(DetectionEngineTest, DetectionReasonContainsExpectedText) {
    const Event event = MakeEvent("powershell.exe", "powershell.exe -enc ZQBjAGgAbwA=");
    const std::vector<Rule> rules = {MakeRule("Encoded PowerShell", "powershell.exe", "-enc")};

    const std::vector<DetectionResult> results = engine_.Evaluate(event, rules);

    ASSERT_EQ(results.size(), 1u);
    const std::string& reason = results.front().Reason();
    EXPECT_NE(reason.find("powershell.exe"), std::string::npos)
        << "Reason should reference the matched process name; was: " << reason;
    EXPECT_NE(reason.find("-enc"), std::string::npos)
        << "Reason should reference the matched command-line fragment; was: " << reason;
}

}  // namespace
}  // namespace sentinelforge
