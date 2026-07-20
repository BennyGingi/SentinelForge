#pragma once

#include <optional>
#include <vector>

#include "DetectionEngine.h"
#include "Event.h"
#include "EventParser.h"
#include "Logger.h"
#include "Rule.h"
#include "RuleLoader.h"

namespace sentinelforge {

// Owns the collector's startup sequence and top-level lifecycle.
// Delegates rule matching entirely to DetectionEngine and rule discovery
// entirely to RuleLoader; Application only loads inputs and logs the outcome.
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
};

}  // namespace sentinelforge
