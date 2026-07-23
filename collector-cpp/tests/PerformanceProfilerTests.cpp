#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>

#include <gtest/gtest.h>

#include "Logger.h"
#include "PerformanceProfiler.h"

namespace sentinelforge {
namespace {

TEST(PerformanceProfilerTest, RecordsMeasurements) {
    PerformanceProfiler profiler;

    profiler.Start("Alpha");
    profiler.Stop("Alpha");

    EXPECT_TRUE(profiler.Has("Alpha")) << "A completed Start/Stop pair must be recorded";
    EXPECT_EQ(profiler.Measurements().size(), 1u);
    EXPECT_EQ(profiler.Measurements().front().name, "Alpha");
}

TEST(PerformanceProfilerTest, ElapsedTimeIsNonNegative) {
    PerformanceProfiler profiler;

    profiler.Start("Work");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    profiler.Stop("Work");

    EXPECT_GE(profiler.Elapsed("Work").count(), 0)
        << "Elapsed time must never be negative";
}

TEST(PerformanceProfilerTest, UnknownStagesHandledSafely) {
    PerformanceProfiler profiler;

    // Stop without Start must not throw or invent a measurement.
    EXPECT_NO_THROW(profiler.Stop("NeverStarted"));
    EXPECT_FALSE(profiler.Has("NeverStarted"));
    EXPECT_EQ(profiler.Elapsed("Missing").count(), 0)
        << "Unknown stages must report 0 ms rather than fail";
}

TEST(PerformanceProfilerTest, MultipleMeasurementsStoredCorrectly) {
    PerformanceProfiler profiler;

    profiler.Start("First");
    profiler.Stop("First");
    profiler.Start("Second");
    profiler.Stop("Second");
    profiler.Start("Third");
    profiler.Stop("Third");

    ASSERT_EQ(profiler.Measurements().size(), 3u);
    EXPECT_EQ(profiler.Measurements()[0].name, "First");
    EXPECT_EQ(profiler.Measurements()[1].name, "Second");
    EXPECT_EQ(profiler.Measurements()[2].name, "Third");
    EXPECT_TRUE(profiler.Has("First"));
    EXPECT_TRUE(profiler.Has("Second"));
    EXPECT_TRUE(profiler.Has("Third"));
}

TEST(PerformanceProfilerTest, TotalRuntimeCalculationWorks) {
    PerformanceProfiler profiler;

    profiler.Start(ProfileStage::Configuration);
    profiler.Stop(ProfileStage::Configuration);
    profiler.Start(ProfileStage::DetectionEngine);
    profiler.Stop(ProfileStage::DetectionEngine);

    profiler.Start(ProfileStage::TotalRuntime);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    profiler.Stop(ProfileStage::TotalRuntime);

    EXPECT_TRUE(profiler.Has(ProfileStage::TotalRuntime));
    EXPECT_GE(profiler.Elapsed(ProfileStage::TotalRuntime).count(), 0);

    // SumOfStages excludes Total Runtime and equals the sum of the other samples.
    const auto sum = profiler.SumOfStages();
    EXPECT_EQ(sum, profiler.Elapsed(ProfileStage::Configuration) +
                       profiler.Elapsed(ProfileStage::DetectionEngine));

    const std::string summary = profiler.FormatSummary();
    EXPECT_NE(summary.find("Performance Summary"), std::string::npos);
    EXPECT_NE(summary.find("Total Runtime"), std::string::npos);
    EXPECT_NE(summary.find("Configuration"), std::string::npos);
}

TEST(PerformanceProfilerTest, LogSummaryUsesLogger) {
    // File-only logger so the test does not depend on console output content,
    // only that LogSummary completes without throwing.
    LoggingSettings settings;
    settings.level = LogLevel::Info;
    settings.console = false;
    settings.file = true;
    settings.path = std::filesystem::temp_directory_path() / "sf_profiler_summary.log";

    Logger logger(settings);
    PerformanceProfiler profiler;
    profiler.Start("Stage");
    profiler.Stop("Stage");
    profiler.Start(ProfileStage::TotalRuntime);
    profiler.Stop(ProfileStage::TotalRuntime);

    EXPECT_NO_THROW(profiler.LogSummary(logger));

    std::error_code ec;
    std::filesystem::remove(settings.path, ec);
}

}  // namespace
}  // namespace sentinelforge
