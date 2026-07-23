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
#ifndef DEFAULT_LOG_PATH
#define DEFAULT_LOG_PATH "logs/sentinelforge.log"
#endif
#ifndef DEFAULT_JSON_EXPORT_FILE
#define DEFAULT_JSON_EXPORT_FILE "detections.json"
#endif
#ifndef DEFAULT_SIGMA_RULES_DIR
#define DEFAULT_SIGMA_RULES_DIR "sigma-rules"
#endif
#ifndef DEFAULT_MONITOR_INPUT_DIR
#define DEFAULT_MONITOR_INPUT_DIR "events/incoming"
#endif
#ifndef DEFAULT_MONITOR_PROCESSED_DIR
#define DEFAULT_MONITOR_PROCESSED_DIR "events/processed"
#endif
#ifndef DEFAULT_MONITOR_FAILED_DIR
#define DEFAULT_MONITOR_FAILED_DIR "events/failed"
#endif

namespace sentinelforge {

namespace {

constexpr std::uint16_t kDefaultApiPort = 8080;
constexpr bool kDefaultDashboardEnabled = false;

std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

LogLevel ParseLogLevel(const std::string& raw) {
    const std::string upper = ToUpper(raw);
    if (upper == "TRACE") return LogLevel::Trace;
    if (upper == "DEBUG") return LogLevel::Debug;
    if (upper == "INFO") return LogLevel::Info;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::Warn;
    if (upper == "ERROR") return LogLevel::Error;
    if (upper == "FATAL") return LogLevel::Fatal;
    throw ConfigurationError(
        "Invalid logging.level '" + raw +
        "' (expected TRACE, DEBUG, INFO, WARN, ERROR, or FATAL)");
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

LoggingSettings ParseLoggingSettings(const nlohmann::json& root,
                                     const std::filesystem::path& base,
                                     const LoggingSettings& defaults) {
    LoggingSettings settings = defaults;
    if (!root.contains("logging")) {
        return settings;
    }

    const auto& logging = root.at("logging");
    if (!logging.is_object()) {
        throw ConfigurationError("Field 'logging' must be a JSON object");
    }

    if (logging.contains("level")) {
        const auto& value = logging.at("level");
        if (!value.is_string()) {
            throw ConfigurationError("Field 'logging.level' must be a string");
        }
        settings.level = ParseLogLevel(value.get<std::string>());
    }

    if (logging.contains("console")) {
        const auto& value = logging.at("console");
        if (!value.is_boolean()) {
            throw ConfigurationError("Field 'logging.console' must be a boolean");
        }
        settings.console = value.get<bool>();
    }

    if (logging.contains("file")) {
        const auto& value = logging.at("file");
        if (!value.is_boolean()) {
            throw ConfigurationError("Field 'logging.file' must be a boolean");
        }
        settings.file = value.get<bool>();
    }

    if (logging.contains("path")) {
        settings.path = ResolvePathField(logging, "path", base, defaults.path);
    }

    return settings;
}

JsonExportSettings ParseJsonExportSettings(const nlohmann::json& root,
                                           const std::filesystem::path& base,
                                           const JsonExportSettings& defaults) {
    JsonExportSettings settings = defaults;
    if (!root.contains("json_export")) {
        return settings;
    }

    const auto& section = root.at("json_export");
    if (!section.is_object()) {
        throw ConfigurationError("Field 'json_export' must be a JSON object");
    }

    if (section.contains("enabled")) {
        const auto& value = section.at("enabled");
        if (!value.is_boolean()) {
            throw ConfigurationError("Field 'json_export.enabled' must be a boolean");
        }
        settings.enabled = value.get<bool>();
    }

    if (section.contains("output_file")) {
        settings.outputFile =
            ResolvePathField(section, "output_file", base, defaults.outputFile);
    }

    return settings;
}

SigmaSettings ParseSigmaSettings(const nlohmann::json& root,
                                 const std::filesystem::path& base,
                                 const SigmaSettings& defaults) {
    SigmaSettings settings = defaults;
    if (!root.contains("sigma")) {
        return settings;
    }

    const auto& section = root.at("sigma");
    if (!section.is_object()) {
        throw ConfigurationError("Field 'sigma' must be a JSON object");
    }

    if (section.contains("enabled")) {
        const auto& value = section.at("enabled");
        if (!value.is_boolean()) {
            throw ConfigurationError("Field 'sigma.enabled' must be a boolean");
        }
        settings.enabled = value.get<bool>();
    }

    if (section.contains("rules_directory")) {
        settings.rulesDirectory =
            ResolvePathField(section, "rules_directory", base, defaults.rulesDirectory);
    }

    return settings;
}

MonitoringSettings ParseMonitoringSettings(const nlohmann::json& root,
                                           const std::filesystem::path& base,
                                           const MonitoringSettings& defaults) {
    MonitoringSettings settings = defaults;
    if (!root.contains("monitoring")) {
        return settings;
    }

    const auto& section = root.at("monitoring");
    if (!section.is_object()) {
        throw ConfigurationError("Field 'monitoring' must be a JSON object");
    }

    if (section.contains("enabled")) {
        const auto& value = section.at("enabled");
        if (!value.is_boolean()) {
            throw ConfigurationError("Field 'monitoring.enabled' must be a boolean");
        }
        settings.enabled = value.get<bool>();
    }

    if (section.contains("input_directory")) {
        settings.inputDirectory =
            ResolvePathField(section, "input_directory", base, defaults.inputDirectory);
    }

    if (section.contains("processed_directory")) {
        settings.processedDirectory =
            ResolvePathField(section, "processed_directory", base, defaults.processedDirectory);
    }

    if (section.contains("failed_directory")) {
        settings.failedDirectory =
            ResolvePathField(section, "failed_directory", base, defaults.failedDirectory);
    }

    if (section.contains("poll_interval_ms")) {
        const auto& value = section.at("poll_interval_ms");
        if (!value.is_number_unsigned()) {
            throw ConfigurationError("Field 'monitoring.poll_interval_ms' must be a non-negative integer");
        }
        const std::uint64_t raw = value.get<std::uint64_t>();
        if (raw < 1 || raw > 3600000) {
            throw ConfigurationError(
                "Field 'monitoring.poll_interval_ms' must be between 1 and 3600000");
        }
        settings.pollIntervalMs = static_cast<std::uint32_t>(raw);
    }

    return settings;
}

}  // namespace

ConfigurationError::ConfigurationError(const std::string& message)
    : std::runtime_error(message) {}

Configuration::Configuration(std::filesystem::path rulesDirectory,
                             std::filesystem::path sampleEventFile,
                             LoggingSettings logging,
                             JsonExportSettings jsonExport,
                             SigmaSettings sigma,
                             MonitoringSettings monitoring,
                             std::filesystem::path outputDirectory,
                             std::uint16_t apiPort,
                             bool dashboardEnabled)
    : rulesDirectory_(std::move(rulesDirectory)),
      sampleEventFile_(std::move(sampleEventFile)),
      logging_(std::move(logging)),
      jsonExport_(std::move(jsonExport)),
      sigma_(std::move(sigma)),
      monitoring_(std::move(monitoring)),
      outputDirectory_(std::move(outputDirectory)),
      apiPort_(apiPort),
      dashboardEnabled_(dashboardEnabled) {}

const std::filesystem::path& Configuration::RulesDirectory() const { return rulesDirectory_; }
const std::filesystem::path& Configuration::SampleEventFile() const { return sampleEventFile_; }
const LoggingSettings& Configuration::Logging() const { return logging_; }
const JsonExportSettings& Configuration::JsonExport() const { return jsonExport_; }
const SigmaSettings& Configuration::Sigma() const { return sigma_; }
const MonitoringSettings& Configuration::Monitoring() const { return monitoring_; }
const std::filesystem::path& Configuration::OutputDirectory() const { return outputDirectory_; }
std::uint16_t Configuration::ApiPort() const { return apiPort_; }
bool Configuration::DashboardEnabled() const { return dashboardEnabled_; }

Configuration Configuration::Defaults() {
    LoggingSettings logging;
    logging.level = LogLevel::Info;
    logging.console = true;
    logging.file = false;
    logging.path = std::filesystem::path(DEFAULT_LOG_PATH);

    JsonExportSettings jsonExport;
    jsonExport.enabled = true;
    jsonExport.outputFile = std::filesystem::path(DEFAULT_JSON_EXPORT_FILE);

    SigmaSettings sigma;
    sigma.enabled = true;
    sigma.rulesDirectory = std::filesystem::path(DEFAULT_SIGMA_RULES_DIR);

    MonitoringSettings monitoring;
    monitoring.enabled = true;
    monitoring.inputDirectory = std::filesystem::path(DEFAULT_MONITOR_INPUT_DIR);
    monitoring.processedDirectory = std::filesystem::path(DEFAULT_MONITOR_PROCESSED_DIR);
    monitoring.failedDirectory = std::filesystem::path(DEFAULT_MONITOR_FAILED_DIR);
    monitoring.pollIntervalMs = 1000;

    return Configuration(std::filesystem::path(DEFAULT_RULES_DIR),
                         std::filesystem::path(DEFAULT_SAMPLE_EVENT_FILE),
                         std::move(logging),
                         std::move(jsonExport),
                         std::move(sigma),
                         std::move(monitoring),
                         std::filesystem::path(DEFAULT_OUTPUT_DIR),
                         kDefaultApiPort,
                         kDefaultDashboardEnabled);
}

Configuration Configuration::LoadFromFile(const std::filesystem::path& path, const Logger& logger) {
    if (!std::filesystem::exists(path)) {
        logger.Warn("Configuration",
                    "Configuration file not found at '" + path.string() +
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

    LoggingSettings logging = ParseLoggingSettings(json, base, defaults.Logging());
    JsonExportSettings jsonExport = ParseJsonExportSettings(json, base, defaults.JsonExport());
    SigmaSettings sigma = ParseSigmaSettings(json, base, defaults.Sigma());
    MonitoringSettings monitoring = ParseMonitoringSettings(json, base, defaults.Monitoring());

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

    return Configuration(std::move(rulesDir), std::move(sampleEvent), std::move(logging),
                         std::move(jsonExport), std::move(sigma), std::move(monitoring),
                         std::move(outputDir), apiPort, dashboard);
}

}  // namespace sentinelforge
