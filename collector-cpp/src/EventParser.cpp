#include "EventParser.h"

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

std::uint32_t RequirePid(const nlohmann::json& json, const std::filesystem::path& path) {
    static constexpr char kField[] = "pid";
    if (!json.contains(kField)) {
        throw std::runtime_error("Missing required field 'pid' in " + path.string());
    }
    if (!json.at(kField).is_number_unsigned()) {
        throw std::runtime_error("Field 'pid' must be a non-negative integer in " + path.string());
    }
    return json.at(kField).get<std::uint32_t>();
}

}  // namespace

Event EventParser::ParseFile(const std::filesystem::path& path) const {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw std::runtime_error("Failed to open telemetry file: " + path.string());
    }

    nlohmann::json json;
    try {
        stream >> json;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Malformed JSON in " + path.string() + ": " + e.what());
    }

    return Event(RequireString(json, "timestamp", path),
                 RequireString(json, "hostname", path),
                 RequireString(json, "username", path),
                 RequireString(json, "process_name", path),
                 RequireString(json, "parent_process", path),
                 RequireString(json, "command_line", path),
                 RequirePid(json, path));
}

}  // namespace sentinelforge
