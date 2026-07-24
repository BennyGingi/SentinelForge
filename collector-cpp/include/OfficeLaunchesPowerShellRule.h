#pragma once

#include <optional>
#include <string>

#include "CorrelationAlert.h"
#include "CorrelationRule.h"

namespace sentinelforge {

// Example behavioral rule: an Office process is followed by powershell.exe
// within 60 seconds. Exists to validate the correlation framework.
class OfficeLaunchesPowerShellRule : public CorrelationRule {
public:
    std::string Id() const override;
    std::optional<CorrelationAlert> Evaluate(
        const NormalizedEvent& current,
        const std::deque<CorrelationHistoryEntry>& history,
        const std::vector<DetectionResult>& detectionResults) const override;
};

}  // namespace sentinelforge
