#pragma once

#include <cstdint>
#include <string>

namespace sentinelforge {

// Immutable representation of a single telemetry event (e.g. a Windows
// process-creation record). Constructed once by EventParser and never
// mutated afterward.
class Event {
public:
    Event(std::string timestamp,
          std::string hostname,
          std::string username,
          std::string processName,
          std::string parentProcess,
          std::string commandLine,
          std::uint32_t pid);

    const std::string& Timestamp() const;
    const std::string& Hostname() const;
    const std::string& Username() const;
    const std::string& ProcessName() const;
    const std::string& ParentProcess() const;
    const std::string& CommandLine() const;
    std::uint32_t Pid() const;

private:
    std::string timestamp_;
    std::string hostname_;
    std::string username_;
    std::string processName_;
    std::string parentProcess_;
    std::string commandLine_;
    std::uint32_t pid_;
};

}  // namespace sentinelforge
