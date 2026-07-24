#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "CorrelationAlert.h"
#include "CorrelationRule.h"
#include "DetectionResult.h"
#include "NormalizedEvent.h"

namespace sentinelforge {

// Runtime settings for the Event Correlation Framework.
struct CorrelationSettings {
    bool enabled = true;
    std::size_t maxEvents = 1000;
    std::uint32_t timeWindowSeconds = 600;
};

// Maintains a rolling history of NormalizedEvent objects and evaluates
// registered CorrelationRule implementations. Independent of JSON parsing.
class CorrelationEngine {
public:
    explicit CorrelationEngine(CorrelationSettings settings = {});

    void Configure(CorrelationSettings settings);
    const CorrelationSettings& Settings() const;

    // Register an additional rule. Ownership transfers to the engine.
    void RegisterRule(std::unique_ptr<CorrelationRule> rule);

    // Clear history and duplicate-suppression state (rules remain registered).
    void ClearHistory();

    // Append event to history (when enabled), prune, evaluate rules, suppress
    // duplicate alerts for identical chains. Returns newly generated alerts.
    std::vector<CorrelationAlert> Process(const NormalizedEvent& event,
                                          const std::vector<DetectionResult>& detectionResults);

    std::size_t HistorySize() const;
    std::size_t RuleCount() const;

private:
    void PruneHistory(std::chrono::system_clock::time_point now);
    void RegisterDefaultRules();
    static std::string Fingerprint(const CorrelationRule& rule, const CorrelationAlert& alert);

    CorrelationSettings settings_;
    std::deque<CorrelationHistoryEntry> history_;
    std::vector<std::unique_ptr<CorrelationRule>> rules_;
    std::unordered_set<std::string> emittedFingerprints_;
};

}  // namespace sentinelforge
