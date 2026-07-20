#pragma once

#include <optional>
#include <vector>

#include "DetectionEngine.h"
#include "Event.h"
#include "EventParser.h"
#include "Logger.h"
#include "ReportPrinter.h"
#include "Rule.h"
#include "RuleLoader.h"

namespace sentinelforge {

// Owns the collector's startup sequence and top-level lifecycle.
// Coordinates the workflow only: loading, detection, and report rendering
// are each delegated to their own class.
class Application {
public:
    Application();

    int Run();

private:
    void PrintBanner() const;
    std::optional<Event> LoadEvent() const;
    std::optional<std::vector<Rule>> LoadRules() const;
    void RunDetection(const Event& event, const std::vector<Rule>& rules) const;

    Logger logger_;
    EventParser eventParser_;
    RuleLoader ruleLoader_;
    DetectionEngine detectionEngine_;
    ReportPrinter reportPrinter_;
};

}  // namespace sentinelforge
