#pragma once

#include "DetectionResult.h"
#include "Event.h"
#include "Rule.h"

namespace sentinelforge {

// Evaluates a single Event against a single Rule. Has no knowledge of JSON,
// files, or logging — pure matching logic only.
class DetectionEngine {
public:
    DetectionResult Evaluate(const Event& event, const Rule& rule) const;
};

}  // namespace sentinelforge
