#pragma once

#include <string>
#include <utility>

namespace sentinelforge {

// Immutable outcome of evaluating one Event against one Rule.
// Small enough to stay header-only (no corresponding .cpp).
class DetectionResult {
public:
    DetectionResult(bool matched,
                    std::string ruleName,
                    std::string severity,
                    std::string mitre,
                    std::string reason)
        : matched_(matched),
          ruleName_(std::move(ruleName)),
          severity_(std::move(severity)),
          mitre_(std::move(mitre)),
          reason_(std::move(reason)) {}

    bool Matched() const { return matched_; }
    const std::string& RuleName() const { return ruleName_; }
    const std::string& Severity() const { return severity_; }
    const std::string& Mitre() const { return mitre_; }
    const std::string& Reason() const { return reason_; }

private:
    bool matched_;
    std::string ruleName_;
    std::string severity_;
    std::string mitre_;
    std::string reason_;
};

}  // namespace sentinelforge
