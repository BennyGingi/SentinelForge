#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "DetectionResult.h"
#include "Event.h"

namespace sentinelforge {

// Immutable snapshot of one detection run: the event that was inspected,
// how many rules were involved, and every DetectionResult produced.
// Pure data — no logging, no printing, no formatting.
class DetectionReport {
public:
    DetectionReport(const Event& event,
                     std::size_t rulesLoaded,
                     std::size_t rulesEvaluated,
                     std::vector<DetectionResult> results);

    const std::string& Timestamp() const;
    const std::string& Hostname() const;
    const std::string& Username() const;
    const std::string& ProcessName() const;
    const std::string& CommandLine() const;

    std::size_t RulesLoaded() const;
    std::size_t RulesEvaluated() const;
    std::size_t MatchesFound() const;

    const std::vector<DetectionResult>& Results() const;

private:
    std::string timestamp_;
    std::string hostname_;
    std::string username_;
    std::string processName_;
    std::string commandLine_;
    std::size_t rulesLoaded_;
    std::size_t rulesEvaluated_;
    std::size_t matchesFound_;
    std::vector<DetectionResult> results_;
};

}  // namespace sentinelforge
