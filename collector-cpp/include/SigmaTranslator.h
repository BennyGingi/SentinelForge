#pragma once

#include "Rule.h"
#include "SigmaRule.h"

namespace sentinelforge {

// Converts a validated SigmaRule into the internal Rule model used by
// DetectionEngine. DetectionEngine never sees Sigma types.
class SigmaTranslator {
public:
    Rule Translate(const SigmaRule& sigmaRule) const;
};

}  // namespace sentinelforge
