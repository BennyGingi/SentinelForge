#include "Logger.h"

#include <iostream>

namespace sentinelforge {

void Logger::Info(std::string_view message) const {
    Log("INFO", message);
}

void Logger::Warning(std::string_view message) const {
    Log("WARNING", message);
}

void Logger::Error(std::string_view message) const {
    Log("ERROR", message);
}

void Logger::Log(std::string_view level, std::string_view message) const {
    std::ostream& stream = (level == "ERROR") ? std::cerr : std::cout;
    stream << "[" << level << "] " << message << "\n";
}

}  // namespace sentinelforge
