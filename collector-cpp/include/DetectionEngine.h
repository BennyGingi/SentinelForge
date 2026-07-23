#pragma once

#include <vector>

#include "DetectionResult.h"
#include "NormalizedEvent.h"
#include "Rule.h"

namespace sentinelforge {

// Evaluates a NormalizedEvent against a set of Rules. Has no knowledge of
// JSON, files, event sources, or logging — pure matching logic only. Returns
// one DetectionResult per rule, in the same order, regardless of match.
class DetectionEngine {
public:
    std::vector<DetectionResult> Evaluate(const NormalizedEvent& event,
                                          const std::vector<Rule>& rules) const;

private:
    DetectionResult EvaluateOne(const NormalizedEvent& event, const Rule& rule) const;
};

}  // namespace sentinelforge
