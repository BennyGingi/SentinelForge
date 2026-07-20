#include "Event.h"

#include <utility>

namespace sentinelforge {

Event::Event(std::string timestamp,
             std::string hostname,
             std::string username,
             std::string processName,
             std::string parentProcess,
             std::string commandLine,
             std::uint32_t pid)
    : timestamp_(std::move(timestamp)),
      hostname_(std::move(hostname)),
      username_(std::move(username)),
      processName_(std::move(processName)),
      parentProcess_(std::move(parentProcess)),
      commandLine_(std::move(commandLine)),
      pid_(pid) {}

const std::string& Event::Timestamp() const { return timestamp_; }
const std::string& Event::Hostname() const { return hostname_; }
const std::string& Event::Username() const { return username_; }
const std::string& Event::ProcessName() const { return processName_; }
const std::string& Event::ParentProcess() const { return parentProcess_; }
const std::string& Event::CommandLine() const { return commandLine_; }
std::uint32_t Event::Pid() const { return pid_; }

}  // namespace sentinelforge
