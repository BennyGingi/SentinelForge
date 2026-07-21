#include "Logger.h"

#include <iostream>

namespace sentinelforge {

namespace {

std::string_view LevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARNING";
        case LogLevel::Error:
            return "ERROR";
    }
    return "INFO";
}

}  // namespace

Logger::Logger(LogLevel minLevel) : minLevel_(minLevel) {}

void Logger::SetMinLevel(LogLevel minLevel) { minLevel_ = minLevel; }
LogLevel Logger::MinLevel() const { return minLevel_; }

void Logger::Debug(std::string_view message) const { Log(LogLevel::Debug, message); }
void Logger::Info(std::string_view message) const { Log(LogLevel::Info, message); }
void Logger::Warning(std::string_view message) const { Log(LogLevel::Warning, message); }
void Logger::Error(std::string_view message) const { Log(LogLevel::Error, message); }

bool Logger::ShouldLog(LogLevel level) const {
    return static_cast<int>(level) >= static_cast<int>(minLevel_);
}

void Logger::Log(LogLevel level, std::string_view message) const {
    if (!ShouldLog(level)) {
        return;
    }
    std::ostream& stream = (level == LogLevel::Error) ? std::cerr : std::cout;
    stream << "[" << LevelName(level) << "] " << message << "\n";
}

}  // namespace sentinelforge
