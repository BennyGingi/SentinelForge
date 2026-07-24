#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "NormalizedEvent.h"

namespace sentinelforge {

// Immutable outcome of a behavioral correlation rule evaluation.
class CorrelationAlert {
public:
    CorrelationAlert(std::string title,
                     std::string description,
                     std::string severity,
                     std::uint8_t confidence,
                     std::string timestamp,
                     std::size_t matchedEventCount,
                     std::vector<std::string> mitreTechniques,
                     std::vector<NormalizedEvent> contributingEvents);

    const std::string& Title() const;
    const std::string& Description() const;
    const std::string& Severity() const;
    std::uint8_t Confidence() const;
    const std::string& Timestamp() const;
    std::size_t MatchedEventCount() const;
    const std::vector<std::string>& MitreTechniques() const;
    const std::vector<NormalizedEvent>& ContributingEvents() const;

private:
    std::string title_;
    std::string description_;
    std::string severity_;
    std::uint8_t confidence_;
    std::string timestamp_;
    std::size_t matchedEventCount_;
    std::vector<std::string> mitreTechniques_;
    std::vector<NormalizedEvent> contributingEvents_;
};

}  // namespace sentinelforge
