#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "JsonExporter.h"
#include "Logger.h"

namespace sentinelforge {

// Thrown when a configuration file is present but contains values the
// collector cannot safely run with: wrong JSON types, out-of-range numbers,
// unknown logging levels, or empty required paths. Signals that the process
// should report the problem and exit cleanly rather than continue with
// nonsense settings.
class ConfigurationError : public std::runtime_error {
public:
    explicit ConfigurationError(const std::string& message);
};

// Immutable, centrally-owned runtime settings for the collector.
//
// There is deliberately no global instance and no singleton: exactly one
// Configuration is created at startup (see LoadFromFile), owned by
// Application, and handed by value or const reference to whatever component
// needs a particular setting. Every accessor is read-only and the object
// never changes after construction.
class Configuration {
public:
    // Loads configuration from `path`:
    //
    //   * File missing        -> returns a Configuration built entirely from
    //                            defaults and logs a warning; the collector
    //                            continues running.
    //   * File present, valid -> returns a Configuration in which file values
    //                            override the defaults.
    //   * File present, bad   -> throws ConfigurationError; the caller is
    //                            expected to report it and exit cleanly.
    //
    // Relative paths inside the file are resolved against the repository root
    // (the parent of the config file's directory) so the same committed
    // config works regardless of the process's working directory.
    static Configuration LoadFromFile(const std::filesystem::path& path, const Logger& logger);

    // A Configuration built purely from compile-time defaults.
    static Configuration Defaults();

    const std::filesystem::path& RulesDirectory() const;
    const std::filesystem::path& SampleEventFile() const;
    const LoggingSettings& Logging() const;
    const JsonExportSettings& JsonExport() const;
    const std::filesystem::path& OutputDirectory() const;
    std::uint16_t ApiPort() const;
    bool DashboardEnabled() const;

private:
    Configuration(std::filesystem::path rulesDirectory,
                  std::filesystem::path sampleEventFile,
                  LoggingSettings logging,
                  JsonExportSettings jsonExport,
                  std::filesystem::path outputDirectory,
                  std::uint16_t apiPort,
                  bool dashboardEnabled);

    std::filesystem::path rulesDirectory_;
    std::filesystem::path sampleEventFile_;
    LoggingSettings logging_;
    JsonExportSettings jsonExport_;
    std::filesystem::path outputDirectory_;
    std::uint16_t apiPort_;
    bool dashboardEnabled_;
};

}  // namespace sentinelforge
