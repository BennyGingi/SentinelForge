#pragma once

#include <vector>

#include "DetectionResult.h"
#include "Event.h"
#include "Rule.h"

namespace sentinelforge {

// Evaluates an Event against a set of Rules. Has no knowledge of JSON,
// files, or logging — pure matching logic only. Returns one DetectionResult
// per rule, in the same order, regardless of whether it matched.
class DetectionEngine {
public:
    std::vector<DetectionResult> Evaluate(const Event& event, const std::vector<Rule>& rules) const;

private:
    DetectionResult EvaluateOne(const Event& event, const Rule& rule) const;
};

}  // namespace sentinelforge
