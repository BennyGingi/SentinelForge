#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "Configuration.h"
#include "DetectionEngine.h"
#include "Event.h"
#include "EventMonitor.h"
#include "EventNormalizer.h"
#include "EventParser.h"
#include "JsonExporter.h"
#include "Logger.h"
#include "NormalizedEvent.h"
#include "PerformanceProfiler.h"
#include "ReportPrinter.h"
#include "Rule.h"
#include "RuleLoader.h"
#include "SigmaLoader.h"

namespace sentinelforge {

// Owns the collector's startup sequence and top-level lifecycle.
// Loads configuration and rules once, then either runs a single sample event
// (monitoring disabled) or delegates continuous processing to EventMonitor.
class Application {
public:
    Application();

    int Run();

private:
    void PrintBanner() const;
    std::optional<Configuration> LoadConfiguration();
    void LogConfiguration(const Configuration& config) const;
    std::optional<Event> LoadEvent(const std::filesystem::path& sampleEventFile);
    std::optional<RuleLoadResult> LoadRules(const Configuration& config);
    void LogRuleLoadResult(const RuleLoadResult& result) const;
    void RunDetection(const NormalizedEvent& event,
                      const std::vector<Rule>& rules,
                      const JsonExportSettings& jsonExport);
    int RunOneShot(const Configuration& config, const std::vector<Rule>& rules);
    int RunMonitoring(const Configuration& config, std::vector<Rule> rules);
    void PrintPerformanceSummary() const;

    Logger logger_;
    PerformanceProfiler profiler_;
    EventParser eventParser_;
    EventNormalizer eventNormalizer_;
    RuleLoader ruleLoader_;
    SigmaLoader sigmaLoader_;
    DetectionEngine detectionEngine_;
    ReportPrinter reportPrinter_;
    JsonExporter jsonExporter_;
};

}  // namespace sentinelforge
