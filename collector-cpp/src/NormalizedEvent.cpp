#include "NormalizedEvent.h"

#include <utility>

namespace sentinelforge {

NormalizedEvent::NormalizedEvent(std::string timestamp,
                                 std::string hostname,
                                 std::string username,
                                 std::string eventType,
                                 std::string processName,
                                 std::string parentProcess,
                                 std::string commandLine,
                                 std::string filePath,
                                 std::string hash,
                                 std::string sourceIp,
                                 std::string destinationIp,
                                 std::string destinationPort,
                                 std::string severity,
                                 std::string provider)
    : timestamp_(std::move(timestamp)),
      hostname_(std::move(hostname)),
      username_(std::move(username)),
      eventType_(std::move(eventType)),
      processName_(std::move(processName)),
      parentProcess_(std::move(parentProcess)),
      commandLine_(std::move(commandLine)),
      filePath_(std::move(filePath)),
      hash_(std::move(hash)),
      sourceIp_(std::move(sourceIp)),
      destinationIp_(std::move(destinationIp)),
      destinationPort_(std::move(destinationPort)),
      severity_(std::move(severity)),
      provider_(std::move(provider)) {}

const std::string& NormalizedEvent::Timestamp() const { return timestamp_; }
const std::string& NormalizedEvent::Hostname() const { return hostname_; }
const std::string& NormalizedEvent::Username() const { return username_; }
const std::string& NormalizedEvent::EventType() const { return eventType_; }
const std::string& NormalizedEvent::ProcessName() const { return processName_; }
const std::string& NormalizedEvent::ParentProcess() const { return parentProcess_; }
const std::string& NormalizedEvent::CommandLine() const { return commandLine_; }
const std::string& NormalizedEvent::FilePath() const { return filePath_; }
const std::string& NormalizedEvent::Hash() const { return hash_; }
const std::string& NormalizedEvent::SourceIp() const { return sourceIp_; }
const std::string& NormalizedEvent::DestinationIp() const { return destinationIp_; }
const std::string& NormalizedEvent::DestinationPort() const { return destinationPort_; }
const std::string& NormalizedEvent::Severity() const { return severity_; }
const std::string& NormalizedEvent::Provider() const { return provider_; }

}  // namespace sentinelforge
