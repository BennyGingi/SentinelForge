#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "Configuration.h"
#include "CorrelationAlert.h"
#include "CorrelationEngine.h"
#include "DetectionEngine.h"
#include "DetectionResult.h"
#include "Logger.h"
#include "NormalizedEvent.h"
#include "Rule.h"

namespace sentinelforge {
namespace {

NormalizedEvent MakeEvent(const std::string& processName,
                          const std::string& parentProcess = "explorer.exe",
                          const std::string& timestamp = "2026-07-20T14:32:07Z",
                          const std::string& hostname = "HOST-01",
                          const std::string& commandLine = "") {
    const std::string cmd =
        commandLine.empty() ? (processName + " -NoProfile") : commandLine;
    return NormalizedEvent(timestamp, hostname, "user", "process_create", processName,
                           parentProcess, cmd, "", "", "", "", "", "", "json");
}

class CorrelationEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        CorrelationSettings settings;
        settings.enabled = true;
        settings.maxEvents = 1000;
        settings.timeWindowSeconds = 600;
        engine_ = CorrelationEngine(settings);
    }

    CorrelationEngine engine_{};
    DetectionEngine detectionEngine_;
};

TEST_F(CorrelationEngineTest, EmptyHistoryProducesNoAlert) {
    const NormalizedEvent ps = MakeEvent("powershell.exe", "explorer.exe");
    const auto alerts = engine_.Process(ps, {});

    EXPECT_TRUE(alerts.empty());
    EXPECT_EQ(engine_.HistorySize(), 1u);
}

TEST_F(CorrelationEngineTest, HistorySizeLimitEnforced) {
    CorrelationSettings settings;
    settings.enabled = true;
    settings.maxEvents = 3;
    settings.timeWindowSeconds = 600;
    engine_.Configure(settings);
    engine_.ClearHistory();

    engine_.Process(MakeEvent("a.exe"), {});
    engine_.Process(MakeEvent("b.exe"), {});
    engine_.Process(MakeEvent("c.exe"), {});
    engine_.Process(MakeEvent("d.exe"), {});

    EXPECT_EQ(engine_.HistorySize(), 3u);
}

TEST_F(CorrelationEngineTest, EventExpirationRemovesOldEntries) {
    CorrelationSettings settings;
    settings.enabled = true;
    settings.maxEvents = 100;
    settings.timeWindowSeconds = 1;
    engine_.Configure(settings);
    engine_.ClearHistory();

    engine_.Process(MakeEvent("winword.exe"), {});
    EXPECT_EQ(engine_.HistorySize(), 1u);

    std::this_thread::sleep_for(std::chrono::milliseconds{1100});

    engine_.Process(MakeEvent("notepad.exe"), {});
    EXPECT_EQ(engine_.HistorySize(), 1u);
}

TEST_F(CorrelationEngineTest, SuccessfulOfficeToPowerShellCorrelation) {
    engine_.ClearHistory();

    const auto officeAlerts = engine_.Process(MakeEvent("WINWORD.EXE"), {});
    EXPECT_TRUE(officeAlerts.empty());

    const auto alerts =
        engine_.Process(MakeEvent("powershell.exe", "WINWORD.EXE", "2026-07-20T14:32:10Z"), {});

    ASSERT_EQ(alerts.size(), 1u);
    EXPECT_EQ(alerts.front().Title(), "Office application launches PowerShell");
    EXPECT_EQ(alerts.front().Severity(), "High");
    EXPECT_GE(alerts.front().Confidence(), 80);
    EXPECT_EQ(alerts.front().MatchedEventCount(), 2u);
    ASSERT_FALSE(alerts.front().MitreTechniques().empty());
    EXPECT_EQ(alerts.front().ContributingEvents().size(), 2u);
}

TEST_F(CorrelationEngineTest, FailedCorrelationWhenNoOfficeParentOrHistory) {
    engine_.ClearHistory();

    engine_.Process(MakeEvent("chrome.exe"), {});
    const auto alerts =
        engine_.Process(MakeEvent("powershell.exe", "chrome.exe"), {});

    EXPECT_TRUE(alerts.empty());
}

TEST_F(CorrelationEngineTest, DuplicateSuppression) {
    engine_.ClearHistory();

    engine_.Process(MakeEvent("WINWORD.EXE", "explorer.exe", "t1"), {});
    const NormalizedEvent ps =
        MakeEvent("powershell.exe", "WINWORD.EXE", "t2", "HOST-01", "powershell.exe -enc X");

    const auto first = engine_.Process(ps, {});
    ASSERT_EQ(first.size(), 1u);

    // Identical contributing chain fingerprints should be suppressed.
    const auto second = engine_.Process(ps, {});
    EXPECT_TRUE(second.empty());
}

TEST_F(CorrelationEngineTest, MultipleUnrelatedEventsDoNotCorrelate) {
    engine_.ClearHistory();

    engine_.Process(MakeEvent("notepad.exe"), {});
    engine_.Process(MakeEvent("chrome.exe"), {});
    engine_.Process(MakeEvent("cmd.exe"), {});
    const auto alerts = engine_.Process(MakeEvent("powershell.exe", "explorer.exe"), {});

    EXPECT_TRUE(alerts.empty());
    EXPECT_EQ(engine_.HistorySize(), 4u);
}

TEST_F(CorrelationEngineTest, DisabledEngineProducesNoAlertsOrHistory) {
    CorrelationSettings settings;
    settings.enabled = false;
    settings.maxEvents = 100;
    settings.timeWindowSeconds = 600;
    engine_.Configure(settings);
    engine_.ClearHistory();

    engine_.Process(MakeEvent("WINWORD.EXE"), {});
    const auto alerts = engine_.Process(MakeEvent("powershell.exe", "WINWORD.EXE"), {});

    EXPECT_TRUE(alerts.empty());
    EXPECT_EQ(engine_.HistorySize(), 0u);
}

TEST_F(CorrelationEngineTest, EngineStabilityWithManyEvents) {
    CorrelationSettings settings;
    settings.enabled = true;
    settings.maxEvents = 50;
    settings.timeWindowSeconds = 600;
    engine_.Configure(settings);
    engine_.ClearHistory();

    for (int i = 0; i < 200; ++i) {
        engine_.Process(MakeEvent("proc" + std::to_string(i) + ".exe"), {});
    }
    EXPECT_EQ(engine_.HistorySize(), 50u);

    engine_.Process(MakeEvent("WINWORD.EXE"), {});
    const auto alerts = engine_.Process(MakeEvent("powershell.exe", "WINWORD.EXE"), {});
    EXPECT_EQ(alerts.size(), 1u);
    EXPECT_LE(engine_.HistorySize(), 50u);
}

TEST_F(CorrelationEngineTest, ParentOnlyCorrelationWithoutOfficeInHistory) {
    engine_.ClearHistory();

    const auto alerts =
        engine_.Process(MakeEvent("powershell.exe", "EXCEL.EXE", "2026-07-20T14:00:00Z"), {});

    ASSERT_EQ(alerts.size(), 1u);
    EXPECT_EQ(alerts.front().MatchedEventCount(), 2u);
}

TEST_F(CorrelationEngineTest, DetectionRegressionStillWorksAlongsideCorrelation) {
    engine_.ClearHistory();

    const NormalizedEvent event =
        MakeEvent("powershell.exe", "explorer.exe", "2026-07-20T14:32:07Z", "HOST",
                  "powershell.exe -enc SGVsbG8=");
    const std::vector<Rule> rules = {
        Rule("Encoded PowerShell", "powershell.exe", "-enc", "High", "T1059.001", "", "", "", "")};

    const std::vector<DetectionResult> results = detectionEngine_.Evaluate(event, rules);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results.front().Matched());

    // No office chain → correlation empty, detection still matched.
    const auto alerts = engine_.Process(event, results);
    EXPECT_TRUE(alerts.empty());
    EXPECT_TRUE(results.front().Matched());
}

TEST_F(CorrelationEngineTest, ConfigurationLoading) {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    const std::filesystem::path tempDir = std::filesystem::temp_directory_path() /
                                          ("sf_corr_cfg_" + std::string(info->name()));
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
    std::filesystem::create_directories(tempDir / "config");

    const std::filesystem::path configPath = tempDir / "config" / "sentinelforge.json";
    {
        std::ofstream out(configPath);
        out << R"({
  "correlation": {
    "enabled": false,
    "max_events": 42,
    "time_window_seconds": 120
  }
})";
    }

    Logger quiet{LoggingSettings{LogLevel::Error, true, false, {}}};
    const Configuration config = Configuration::LoadFromFile(configPath, quiet);

    EXPECT_FALSE(config.Correlation().enabled);
    EXPECT_EQ(config.Correlation().maxEvents, 42u);
    EXPECT_EQ(config.Correlation().timeWindowSeconds, 120u);

    std::filesystem::remove_all(tempDir, ec);
}

TEST_F(CorrelationEngineTest, DefaultConfigurationIncludesCorrelation) {
    const Configuration config = Configuration::Defaults();
    EXPECT_TRUE(config.Correlation().enabled);
    EXPECT_EQ(config.Correlation().maxEvents, 1000u);
    EXPECT_EQ(config.Correlation().timeWindowSeconds, 600u);
}

TEST_F(CorrelationEngineTest, RegistersAtLeastOneDefaultRule) {
    EXPECT_GE(engine_.RuleCount(), 1u);
}

}  // namespace
}  // namespace sentinelforge
