#pragma once

#include <nlohmann/json.hpp>

#include "Event.h"
#include "NormalizedEvent.h"

namespace sentinelforge {

// Converts source-specific events into a NormalizedEvent for DetectionEngine.
// Today the only production path is JSON → Event → NormalizedEvent; additional
// Normalize* overloads can be added later for Sysmon, EVTX, auditd, etc.
class EventNormalizer {
public:
    // Map a parsed JSON Event into the generic NormalizedEvent model.
    // Fields not present on Event remain empty. Does not throw for empty values.
    NormalizedEvent Normalize(const Event& event) const;

    // Map known keys from a JSON object. Missing keys stay empty; unknown keys
    // are ignored. Useful for optional enrichment and unit tests.
    NormalizedEvent NormalizeJson(const nlohmann::json& json) const;
};

}  // namespace sentinelforge
