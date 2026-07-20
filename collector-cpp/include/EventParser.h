#pragma once

#include <filesystem>

#include "Event.h"

namespace sentinelforge {

// Reads a telemetry JSON file from disk and converts it into an Event.
// Throws std::runtime_error with a descriptive message on any failure:
// unreadable file, malformed JSON, or missing/invalid required fields.
class EventParser {
public:
    Event ParseFile(const std::filesystem::path& path) const;
};

}  // namespace sentinelforge
