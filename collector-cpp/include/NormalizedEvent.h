#pragma once

#include <string>

namespace sentinelforge {

// Source-agnostic security event consumed by DetectionEngine.
// Fields not supplied by a given telemetry source remain empty.
class NormalizedEvent {
public:
    NormalizedEvent() = default;

    NormalizedEvent(std::string timestamp,
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
                    std::string provider);

    const std::string& Timestamp() const;
    const std::string& Hostname() const;
    const std::string& Username() const;
    const std::string& EventType() const;
    const std::string& ProcessName() const;
    const std::string& ParentProcess() const;
    const std::string& CommandLine() const;
    const std::string& FilePath() const;
    const std::string& Hash() const;
    const std::string& SourceIp() const;
    const std::string& DestinationIp() const;
    const std::string& DestinationPort() const;
    const std::string& Severity() const;
    const std::string& Provider() const;

private:
    std::string timestamp_;
    std::string hostname_;
    std::string username_;
    std::string eventType_;
    std::string processName_;
    std::string parentProcess_;
    std::string commandLine_;
    std::string filePath_;
    std::string hash_;
    std::string sourceIp_;
    std::string destinationIp_;
    std::string destinationPort_;
    std::string severity_;
    std::string provider_;
};

}  // namespace sentinelforge
