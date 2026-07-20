#pragma once

#include <filesystem>

#include "Rule.h"

namespace sentinelforge {

// Reads a detection rule JSON file from disk and converts it into a Rule.
// Throws std::runtime_error with a descriptive message on any failure:
// unreadable file, malformed JSON, or missing/invalid required fields.
// Has no knowledge of Event or matching logic.
class RuleParser {
public:
    Rule ParseFile(const std::filesystem::path& path) const;
};

}  // namespace sentinelforge
