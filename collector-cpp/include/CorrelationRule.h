#pragma once

#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "CorrelationAlert.h"
#include "DetectionResult.h"
#include "NormalizedEvent.h"

namespace sentinelforge {

// One observed event retained for correlation, with the wall-clock time it
// entered the CorrelationEngine history (used for expiry and rule windows).
struct CorrelationHistoryEntry {
    NormalizedEvent event;
    std::chrono::system_clock::time_point observedAt{};
};

// Abstract behavioral correlation rule. Concrete rules inspect the current
// event plus recent history and optionally return a CorrelationAlert.
// CorrelationEngine owns registered rules and never embeds rule-specific logic.
class CorrelationRule {
public:
    virtual ~CorrelationRule() = default;

    // Stable identifier used for duplicate-alert suppression.
    virtual std::string Id() const = 0;

    // Evaluate against the current event and recent history. Return nullopt
    // when the rule does not fire. Detection results are provided for future
    // hybrid rules; behavioral rules may ignore them.
    virtual std::optional<CorrelationAlert> Evaluate(
        const NormalizedEvent& current,
        const std::deque<CorrelationHistoryEntry>& history,
        const std::vector<DetectionResult>& detectionResults) const = 0;
};

}  // namespace sentinelforge
