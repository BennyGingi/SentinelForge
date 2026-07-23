#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "Configuration.h"
#include "DetectionEngine.h"
#include "Event.h"
#include "EventParser.h"
#include "JsonExporter.h"
#include "Logger.h"
#include "PerformanceProfiler.h"
#include "ReportPrinter.h"
#include "Rule.h"
#include "RuleLoader.h"

namespace sentinelforge {

// Owns the collector's startup sequence and top-level lifecycle.
// Coordinates the workflow only: configuration loading, telemetry loading,
// detection, and report rendering are each delegated to their own class.
//
// Application owns the single Configuration instance for the process and
// passes each collaborator only the settings it actually needs. Timing of
// each stage is delegated entirely to PerformanceProfiler.
class Application {
public:
    Application();

    int Run();

private:
    void PrintBanner() const;
    std::optional<Configuration> LoadConfiguration();
    void LogConfiguration(const Configuration& config) const;
    std::optional<Event> LoadEvent(const std::filesystem::path& sampleEventFile);
    std::optional<RuleLoadResult> LoadRules(const std::filesystem::path& rulesDirectory);
    void LogRuleLoadResult(const RuleLoadResult& result) const;
    void RunDetection(const Event& event,
                      const std::vector<Rule>& rules,
                      const JsonExportSettings& jsonExport);
    void PrintPerformanceSummary() const;

    Logger logger_;
    PerformanceProfiler profiler_;
    EventParser eventParser_;
    RuleLoader ruleLoader_;
    DetectionEngine detectionEngine_;
    ReportPrinter reportPrinter_;
    JsonExporter jsonExporter_;
};

}  // namespace sentinelforge
