#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>
#include <utility>

namespace sentinelforge {

namespace {

std::string_view LevelLabel(LogLevel level) {
    // Fixed width of 5 so the bracketed level column aligns: [INFO ], [WARN ].
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO ";
        case LogLevel::Warn:
            return "WARN ";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
    }
    return "INFO ";
}

std::string FormatTimestamp(const std::chrono::system_clock::time_point& now) {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    1000;
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);

    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &seconds);
#else
    localtime_r(&seconds, &local);
#endif

    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << '.'
        << std::setw(3) << std::setfill('0') << static_cast<int>(ms.count());
    return out.str();
}

LoggingSettings Sanitize(LoggingSettings settings) {
    if (!settings.console && !settings.file) {
        settings.console = true;
    }
    if (settings.file && settings.path.empty()) {
        settings.file = false;
        settings.console = true;
    }
    return settings;
}

}  // namespace

Logger::Logger() : Logger(LoggingSettings{}) {}

Logger::Logger(LoggingSettings settings) : settings_(Sanitize(std::move(settings))) {
    if (settings_.file) {
        OpenFileStream();
    }
}

void Logger::Configure(LoggingSettings settings) {
    std::lock_guard<std::mutex> lock(mutex_);
    CloseFileStream();
    settings_ = Sanitize(std::move(settings));
    if (settings_.file) {
        OpenFileStream();
        if (!fileStream_.is_open()) {
            settings_.file = false;
            settings_.console = true;
        }
    }
}

LogLevel Logger::MinLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return settings_.level;
}

bool Logger::ConsoleEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return settings_.console;
}

bool Logger::FileEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return settings_.file;
}

const std::filesystem::path& Logger::FilePath() const {
    // Returned by reference while unlocked; callers must not race Configure.
    // Sufficient for Application's single-threaded configure-then-run pattern
    // and for tests that inspect the path after construction.
    return settings_.path;
}

bool Logger::ShouldLog(LogLevel level) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(level) >= static_cast<int>(settings_.level);
}

void Logger::Trace(std::string_view component, std::string_view message) const {
    Emit(LogLevel::Trace, component, message);
}
void Logger::Debug(std::string_view component, std::string_view message) const {
    Emit(LogLevel::Debug, component, message);
}
void Logger::Info(std::string_view component, std::string_view message) const {
    Emit(LogLevel::Info, component, message);
}
void Logger::Warn(std::string_view component, std::string_view message) const {
    Emit(LogLevel::Warn, component, message);
}
void Logger::Error(std::string_view component, std::string_view message) const {
    Emit(LogLevel::Error, component, message);
}
void Logger::Fatal(std::string_view component, std::string_view message) const {
    Emit(LogLevel::Fatal, component, message);
}

void Logger::Emit(LogLevel level, std::string_view component, std::string_view message) const {
    // Cheap early-out: no lock, no timestamp, no string work when filtered.
    if (!ShouldLog(level)) {
        return;
    }

    const std::string line = FormatTimestamp(std::chrono::system_clock::now()) + " [" +
                             std::string(LevelLabel(level)) + "] [" + std::string(component) +
                             "] " + std::string(message) + "\n";

    std::lock_guard<std::mutex> lock(mutex_);

    if (settings_.console) {
        std::ostream& stream =
            (level == LogLevel::Error || level == LogLevel::Fatal) ? std::cerr : std::cout;
        stream << line;
        stream.flush();
    }

    if (settings_.file && fileStream_.is_open()) {
        fileStream_ << line;
        fileStream_.flush();
    }
}

void Logger::OpenFileStream() {
    const std::filesystem::path parent = settings_.path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }

    fileStream_.open(settings_.path, std::ios::out | std::ios::app);
    if (!fileStream_.is_open()) {
        settings_.file = false;
        settings_.console = true;
    }
}

void Logger::CloseFileStream() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

}  // namespace sentinelforge
