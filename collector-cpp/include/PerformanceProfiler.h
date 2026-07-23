#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Logger.h"

namespace sentinelforge {

// One completed, named timing sample. Elapsed is wall time between the
// matching Start/Stop pair, measured with steady_clock.
struct TimingMeasurement {
    std::string name;
    std::chrono::milliseconds elapsed{0};
};

// Well-known stage labels used by Application. Additional names may be
// started/stopped freely without changing the profiler itself.
namespace ProfileStage {
inline constexpr const char* Configuration = "Configuration";
inline constexpr const char* RuleLoading = "Rule Loading";
inline constexpr const char* RuleValidation = "Rule Validation";
inline constexpr const char* EventLoading = "Event Loading";
inline constexpr const char* DetectionEngine = "Detection Engine";
inline constexpr const char* ReportGeneration = "Report Generation";
inline constexpr const char* TotalRuntime = "Total Runtime";
inline constexpr const char* ParseTime = "Parse time";
inline constexpr const char* DetectionTime = "Detection time";
inline constexpr const char* ReportGenerationTime = "Report generation time";
inline constexpr const char* JsonExportTime = "JSON export time";
inline constexpr const char* TotalProcessing = "Total processing time";
}  // namespace ProfileStage

// Encapsulates all timing for the collector. Callers mark stages with
// Start/Stop; they never touch clocks directly. Unknown Stop/Elapsed
// requests are safe no-ops (Elapsed returns 0). Future stages are just
// new string names — no profiler code changes required.
class PerformanceProfiler {
public:
    void Start(std::string_view stage);
    void Stop(std::string_view stage);

    // Clears open timers and completed measurements so the same profiler
    // instance can time successive live events independently.
    void Clear();

    bool Has(std::string_view stage) const;
    std::chrono::milliseconds Elapsed(std::string_view stage) const;
    const std::vector<TimingMeasurement>& Measurements() const;

    // Sum of every completed measurement except total-runtime style stages.
    std::chrono::milliseconds SumOfStages() const;

    // Multi-line professional summary suitable for logging.
    std::string FormatSummary() const;

    // Emits FormatSummary() one line at a time through the structured logger.
    void LogSummary(const Logger& logger) const;

private:
    using Clock = std::chrono::steady_clock;

    std::unordered_map<std::string, Clock::time_point> openTimers_;
    std::vector<TimingMeasurement> measurements_;
    std::unordered_map<std::string, std::size_t> measurementIndex_;
};

}  // namespace sentinelforge
