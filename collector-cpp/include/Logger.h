#pragma once

#include <string_view>

namespace sentinelforge {

// Severity of a log message, ordered from most to least verbose. Used both
// to tag individual messages and to express the minimum severity a Logger
// will emit.
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
};

// Minimal console logger with severity levels. Messages below the logger's
// configured minimum level are suppressed. The minimum level defaults to
// Info so that warnings and errors emitted before configuration is applied
// (for example, a missing config file) are always visible.
class Logger {
public:
    Logger() = default;
    explicit Logger(LogLevel minLevel);

    void SetMinLevel(LogLevel minLevel);
    LogLevel MinLevel() const;

    void Debug(std::string_view message) const;
    void Info(std::string_view message) const;
    void Warning(std::string_view message) const;
    void Error(std::string_view message) const;

private:
    bool ShouldLog(LogLevel level) const;
    void Log(LogLevel level, std::string_view message) const;

    LogLevel minLevel_ = LogLevel::Info;
};

}  // namespace sentinelforge
