#pragma once

#include <optional>

#include "DetectionEngine.h"
#include "Event.h"
#include "EventParser.h"
#include "Logger.h"
#include "Rule.h"
#include "RuleParser.h"

namespace sentinelforge {

// Owns the collector's startup sequence and top-level lifecycle.
// Delegates rule matching entirely to DetectionEngine; Application only
// loads inputs and logs the outcome.
class Application {
public:
    Application();

    int Run();

private:
    void PrintBanner() const;
    std::optional<Event> LoadEvent() const;
    std::optional<Rule> LoadRule() const;
    void RunDetection(const Event& event, const Rule& rule) const;

    Logger logger_;
    EventParser eventParser_;
    RuleParser ruleParser_;
    DetectionEngine detectionEngine_;
};

}  // namespace sentinelforge
