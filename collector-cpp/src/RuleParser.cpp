#include "RuleParser.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace sentinelforge {

namespace {

std::string RequireString(const nlohmann::json& json,
                           const std::string& field,
                           const std::filesystem::path& path) {
    if (!json.contains(field)) {
        throw std::runtime_error("Missing required field '" + field + "' in " + path.string());
    }
    if (!json.at(field).is_string()) {
        throw std::runtime_error("Field '" + field + "' must be a string in " + path.string());
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

    return Rule(RequireString(json, "rule_name", path),
                RequireString(json, "process_name", path),
                RequireString(json, "command_line_contains", path),
                RequireString(json, "severity", path),
                RequireString(json, "mitre", path));
}

}  // namespace sentinelforge
