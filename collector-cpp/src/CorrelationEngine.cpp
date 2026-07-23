#include "CorrelationEngine.h"

#include <utility>

#include "OfficeLaunchesPowerShellRule.h"

namespace sentinelforge {

CorrelationEngine::CorrelationEngine(CorrelationSettings settings)
    : settings_(std::move(settings)) {
    RegisterDefaultRules();
}

void CorrelationEngine::Configure(CorrelationSettings settings) {
    settings_ = std::move(settings);
    PruneHistory(std::chrono::system_clock::now());
}

const CorrelationSettings& CorrelationEngine::Settings() const { return settings_; }

void CorrelationEngine::RegisterRule(std::unique_ptr<CorrelationRule> rule) {
    if (rule) {
        rules_.push_back(std::move(rule));
    }
}

void CorrelationEngine::ClearHistory() {
    history_.clear();
    emittedFingerprints_.clear();
}

std::size_t CorrelationEngine::HistorySize() const { return history_.size(); }

std::size_t CorrelationEngine::RuleCount() const { return rules_.size(); }

void CorrelationEngine::RegisterDefaultRules() {
    rules_.push_back(std::make_unique<OfficeLaunchesPowerShellRule>());
}

void CorrelationEngine::PruneHistory(std::chrono::system_clock::time_point now) {
    const auto window = std::chrono::seconds{settings_.timeWindowSeconds};

    while (!history_.empty() && (now - history_.front().observedAt) > window) {
        history_.pop_front();
    }

    while (history_.size() > settings_.maxEvents) {
        history_.pop_front();
    }
}

std::string CorrelationEngine::Fingerprint(const CorrelationRule& rule,
                                           const CorrelationAlert& alert) {
    std::string fingerprint = rule.Id();
    fingerprint.push_back('|');
    fingerprint.append(alert.Title());
    fingerprint.push_back('|');
    fingerprint.append(alert.Timestamp());
    fingerprint.push_back('|');

    for (const auto& event : alert.ContributingEvents()) {
        fingerprint.append(event.ProcessName());
        fingerprint.push_back('@');
        fingerprint.append(event.Timestamp());
        fingerprint.push_back(';');
        fingerprint.append(event.Hostname());
        fingerprint.push_back(';');
        fingerprint.append(event.CommandLine());
        fingerprint.push_back('|');
    }
    return fingerprint;
}

std::vector<CorrelationAlert> CorrelationEngine::Process(
    const NormalizedEvent& event,
    const std::vector<DetectionResult>& detectionResults) {
    std::vector<CorrelationAlert> alerts;
    if (!settings_.enabled) {
        return alerts;
    }

    const auto now = std::chrono::system_clock::now();
    PruneHistory(now);

    // Snapshot history before appending the current event so rules see prior
    // events only; the current event is passed separately.
    const std::deque<CorrelationHistoryEntry> priorHistory = history_;

    for (const auto& rule : rules_) {
        std::optional<CorrelationAlert> alert = rule->Evaluate(event, priorHistory, detectionResults);
        if (!alert.has_value()) {
            continue;
        }

        const std::string fingerprint = Fingerprint(*rule, *alert);
        if (emittedFingerprints_.count(fingerprint) != 0) {
            continue;
        }
        emittedFingerprints_.insert(fingerprint);
        alerts.push_back(std::move(*alert));
    }

    history_.push_back(CorrelationHistoryEntry{event, now});
    PruneHistory(now);

    // Bound fingerprint set roughly with history growth.
    if (emittedFingerprints_.size() > settings_.maxEvents * 2) {
        emittedFingerprints_.clear();
    }

    return alerts;
}

}  // namespace sentinelforge
