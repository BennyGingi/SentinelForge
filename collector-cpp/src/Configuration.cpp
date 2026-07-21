#include "Configuration.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

// Compile-time defaults. CMake injects absolute paths (resolved at configure
// time) so the "missing config" path works regardless of the process's
// working directory. The fallbacks below keep the file self-contained if it
// is ever compiled without those definitions.
#ifndef DEFAULT_RULES_DIR
#define DEFAULT_RULES_DIR "rules"
#endif
#ifndef DEFAULT_SAMPLE_EVENT_FILE
#define DEFAULT_SAMPLE_EVENT_FILE "sample-logs/process_create.json"
#endif
#ifndef DEFAULT_OUTPUT_DIR
#define DEFAULT_OUTPUT_DIR "output"
#endif

namespace sentinelforge {

namespace {

constexpr std::uint16_t kDefaultApiPort = 8080;
constexpr bool kDefaultDashboardEnabled = false;
constexpr LogLevel kDefaultLoggingLevel = LogLevel::Info;

std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

LogLevel ParseLogLevel(const std::string& raw) {
    const std::string upper = ToUpper(raw);
    if (upper == "DEBUG") return LogLevel::Debug;
    if (upper == "INFO") return LogLevel::Info;
    if (upper == "WARNING") return LogLevel::Warning;
    if (upper == "ERROR") return LogLevel::Error;
    throw ConfigurationError("Invalid logging_level '" + raw +
                             "' (expected DEBUG, INFO, WARNING, or ERROR)");
}

// A path setting must be a non-empty string when present. Relative values are
// resolved against `base` (the repository root); absolute values are taken as
// is. Absent values fall back to the supplied default.
std::filesystem::path ResolvePathField(const nlohmann::json& json,
                                       const std::string& field,
                                       const std::filesystem::path& base,
                                       const std::filesystem::path& fallback) {
    if (!json.contains(field)) {
        return fallback;
    }
    const auto& value = json.at(field);
    if (!value.is_string()) {
        throw ConfigurationError("Field '" + field + "' must be a string");
    }
    const std::string raw = value.get<std::string>();
    if (raw.empty()) {
        throw ConfigurationError("Field '" + field + "' must not be empty");
    }
    const std::filesystem::path candidate(raw);
    return candidate.is_absolute() ? candidate : base / candidate;
}

}  // namespace

ConfigurationError::ConfigurationError(const std::string& message)
    : std::runtime_error(message) {}

Configuration::Configuration(std::filesystem::path rulesDirectory,
                             std::filesystem::path sampleEventFile,
                             LogLevel loggingLevel,
                             std::filesystem::path outputDirectory,
                             std::uint16_t apiPort,
                             bool dashboardEnabled)
    : rulesDirectory_(std::move(rulesDirectory)),
      sampleEventFile_(std::move(sampleEventFile)),
      loggingLevel_(loggingLevel),
      outputDirectory_(std::move(outputDirectory)),
      apiPort_(apiPort),
      dashboardEnabled_(dashboardEnabled) {}

const std::filesystem::path& Configuration::RulesDirectory() const { return rulesDirectory_; }
const std::filesystem::path& Configuration::SampleEventFile() const { return sampleEventFile_; }
LogLevel Configuration::LoggingLevel() const { return loggingLevel_; }
const std::filesystem::path& Configuration::OutputDirectory() const { return outputDirectory_; }
std::uint16_t Configuration::ApiPort() const { return apiPort_; }
bool Configuration::DashboardEnabled() const { return dashboardEnabled_; }

Configuration Configuration::Defaults() {
    return Configuration(std::filesystem::path(DEFAULT_RULES_DIR),
                         std::filesystem::path(DEFAULT_SAMPLE_EVENT_FILE),
                         kDefaultLoggingLevel,
                         std::filesystem::path(DEFAULT_OUTPUT_DIR),
                         kDefaultApiPort,
                         kDefaultDashboardEnabled);
}

Configuration Configuration::LoadFromFile(const std::filesystem::path& path, const Logger& logger) {
    if (!std::filesystem::exists(path)) {
        logger.Warning("Configuration file not found at '" + path.string() +
                       "'; using built-in defaults");
        return Defaults();
    }

    std::ifstream stream(path);
    if (!stream.is_open()) {
        throw ConfigurationError("Failed to open configuration file: " + path.string());
    }

    nlohmann::json json;
    try {
        stream >> json;
    } catch (const nlohmann::json::parse_error& e) {
        throw ConfigurationError("Malformed JSON in " + path.string() + ": " + e.what());
    }

    if (!json.is_object()) {
        throw ConfigurationError("Configuration root must be a JSON object in " + path.string());
    }

    // Repository root: the parent of the config file's own directory.
    const std::filesystem::path base = path.parent_path().parent_path();
    const Configuration defaults = Defaults();

    std::filesystem::path rulesDir =
        ResolvePathField(json, "rules_directory", base, defaults.RulesDirectory());
    std::filesystem::path sampleEvent =
        ResolvePathField(json, "sample_event_file", base, defaults.SampleEventFile());
    std::filesystem::path outputDir =
        ResolvePathField(json, "output_directory", base, defaults.OutputDirectory());

    LogLevel level = defaults.LoggingLevel();
    if (json.contains("logging_level")) {
        const auto& value = json.at("logging_level");
        if (!value.is_string()) {
            throw ConfigurationError("Field 'logging_level' must be a string");
        }
        level = ParseLogLevel(value.get<std::string>());
    }

    std::uint16_t apiPort = defaults.ApiPort();
    if (json.contains("api_port")) {
        const auto& value = json.at("api_port");
        if (!value.is_number_unsigned()) {
            throw ConfigurationError("Field 'api_port' must be a non-negative integer");
        }
        const std::uint64_t raw = value.get<std::uint64_t>();
        if (raw < 1 || raw > 65535) {
            throw ConfigurationError("Field 'api_port' must be between 1 and 65535");
        }
        apiPort = static_cast<std::uint16_t>(raw);
    }

    bool dashboard = defaults.DashboardEnabled();
    if (json.contains("dashboard_enabled")) {
        const auto& value = json.at("dashboard_enabled");
        if (!value.is_boolean()) {
            throw ConfigurationError("Field 'dashboard_enabled' must be a boolean");
        }
        dashboard = value.get<bool>();
    }

    return Configuration(std::move(rulesDir), std::move(sampleEvent), level,
                         std::move(outputDir), apiPort, dashboard);
}

}  // namespace sentinelforge
