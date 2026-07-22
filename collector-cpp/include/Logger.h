#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace sentinelforge {

// Severity of a log message, ordered from most to least verbose. Used both
// to tag individual messages and to express the minimum severity a Logger
// will emit. Messages strictly below the configured level are discarded
// before any formatting or I/O occurs.
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

// Runtime logging settings. Owned by Configuration and applied to Logger via
// Configure(); Logger never reads Configuration itself, keeping the
// dependency arrow Application → Logger (never the reverse).
struct LoggingSettings {
    LogLevel level = LogLevel::Info;
    bool console = true;
    bool file = false;
    std::filesystem::path path{"logs/sentinelforge.log"};
};

// Production-quality structured logger.
//
// Format of every emitted line:
//   YYYY-MM-DD HH:MM:SS.mmm [LEVEL] [Component] Message
//
// Destinations (console, file, or both) are selected by LoggingSettings.
// Writes are serialized with a mutex so concurrent callers never interleave
// lines. The object is neither copyable nor movable (mutex + open stream).
class Logger {
public:
    Logger();
    explicit Logger(LoggingSettings settings);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Reconfigure destinations and minimum level. If neither console nor file
    // is enabled, console is forced on so the process is never silent. If
    // file output is requested but the path is empty or cannot be opened,
    // file output is disabled and console is ensured.
    void Configure(LoggingSettings settings);

    LogLevel MinLevel() const;
    bool ConsoleEnabled() const;
    bool FileEnabled() const;
    const std::filesystem::path& FilePath() const;

    // True when a message at `level` would be emitted. Callers that build
    // expensive strings should check this first and skip the work when false.
    bool ShouldLog(LogLevel level) const;

    void Trace(std::string_view component, std::string_view message) const;
    void Debug(std::string_view component, std::string_view message) const;
    void Info(std::string_view component, std::string_view message) const;
    void Warn(std::string_view component, std::string_view message) const;
    void Error(std::string_view component, std::string_view message) const;
    void Fatal(std::string_view component, std::string_view message) const;

private:
    void Emit(LogLevel level, std::string_view component, std::string_view message) const;
    void OpenFileStream();
    void CloseFileStream();

    LoggingSettings settings_;
    mutable std::mutex mutex_;
    mutable std::ofstream fileStream_;
};

}  // namespace sentinelforge
