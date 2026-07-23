#include "EventNormalizer.h"

#include <string>

namespace sentinelforge {

namespace {

constexpr const char* kJsonProvider = "json";

std::string OptionalString(const nlohmann::json& json, const char* field) {
    if (!json.contains(field) || json.at(field).is_null()) {
        return {};
    }
    if (json.at(field).is_string()) {
        return json.at(field).get<std::string>();
    }
    if (json.at(field).is_number_integer() || json.at(field).is_number_unsigned()) {
        return std::to_string(json.at(field).get<long long>());
    }
    return {};
}

}  // namespace

NormalizedEvent EventNormalizer::Normalize(const Event& event) const {
    return NormalizedEvent(event.Timestamp(),
                           event.Hostname(),
                           event.Username(),
                           /*eventType=*/"",
                           event.ProcessName(),
                           event.ParentProcess(),
                           event.CommandLine(),
                           /*filePath=*/"",
                           /*hash=*/"",
                           /*sourceIp=*/"",
                           /*destinationIp=*/"",
                           /*destinationPort=*/"",
                           /*severity=*/"",
                           kJsonProvider);
}

NormalizedEvent EventNormalizer::NormalizeJson(const nlohmann::json& json) const {
    if (!json.is_object()) {
        return NormalizedEvent{};
    }

    std::string provider = OptionalString(json, "provider");
    if (provider.empty()) {
        provider = kJsonProvider;
    }

    return NormalizedEvent(OptionalString(json, "timestamp"),
                           OptionalString(json, "hostname"),
                           OptionalString(json, "username"),
                           OptionalString(json, "event_type"),
                           OptionalString(json, "process_name"),
                           OptionalString(json, "parent_process"),
                           OptionalString(json, "command_line"),
                           OptionalString(json, "file_path"),
                           OptionalString(json, "hash"),
                           OptionalString(json, "source_ip"),
                           OptionalString(json, "destination_ip"),
                           OptionalString(json, "destination_port"),
                           OptionalString(json, "severity"),
                           std::move(provider));
}

}  // namespace sentinelforge
