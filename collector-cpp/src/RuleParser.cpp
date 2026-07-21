#include "RuleParser.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace sentinelforge {

namespace {

// Returns the field's string value, or "" if the field is absent or not a
// string. Presence/emptiness/type validity of required fields is
// RuleValidator's responsibility, not the parser's — the parser's job is
// only to turn file bytes into a best-effort Rule.
std::string OptionalString(const nlohmann::json& json, const std::string& field) {
    if (!json.contains(field) || !json.at(field).is_string()) {
        return "";
    }
    return json.at(field).get<std::string>();
}

}  // namespace

Rule RuleParser::ParseFile(const std::filesystem::path& path) const {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("Failed to open rule file: " + path.string());
    }

    nlohmann::json json;
    try {
        stream >> json;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Malformed JSON in " + path.string() + ": " + e.what());
    }

    return Rule(OptionalString(json, "rule_name"),
                OptionalString(json, "process_name"),
                OptionalString(json, "command_line_contains"),
                OptionalString(json, "severity"),
                OptionalString(json, "mitre"),
                OptionalString(json, "author"),
                OptionalString(json, "version"),
                OptionalString(json, "description"),
                OptionalString(json, "created_date"));
}

}  // namespace sentinelforge
