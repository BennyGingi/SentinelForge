#pragma once

#include <string_view>

namespace sentinelforge {

// Minimal stateless logger for console output with severity levels.
class Logger {
public:
    void Info(std::string_view message) const;
    void Warning(std::string_view message) const;
    void Error(std::string_view message) const;

private:
    void Log(std::string_view level, std::string_view message) const;
};

}  // namespace sentinelforge
