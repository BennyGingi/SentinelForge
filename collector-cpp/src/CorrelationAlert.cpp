#include "CorrelationAlert.h"

#include <utility>

namespace sentinelforge {

CorrelationAlert::CorrelationAlert(std::string title,
                                   std::string description,
                                   std::string severity,
                                   std::uint8_t confidence,
                                   std::string timestamp,
                                   std::size_t matchedEventCount,
                                   std::vector<std::string> mitreTechniques,
                                   std::vector<NormalizedEvent> contributingEvents)
    : title_(std::move(title)),
      description_(std::move(description)),
      severity_(std::move(severity)),
      confidence_(confidence > 100 ? static_cast<std::uint8_t>(100) : confidence),
      timestamp_(std::move(timestamp)),
      matchedEventCount_(matchedEventCount),
      mitreTechniques_(std::move(mitreTechniques)),
      contributingEvents_(std::move(contributingEvents)) {}

const std::string& CorrelationAlert::Title() const { return title_; }
const std::string& CorrelationAlert::Description() const { return description_; }
const std::string& CorrelationAlert::Severity() const { return severity_; }
std::uint8_t CorrelationAlert::Confidence() const { return confidence_; }
const std::string& CorrelationAlert::Timestamp() const { return timestamp_; }
std::size_t CorrelationAlert::MatchedEventCount() const { return matchedEventCount_; }
const std::vector<std::string>& CorrelationAlert::MitreTechniques() const {
    return mitreTechniques_;
}
const std::vector<NormalizedEvent>& CorrelationAlert::ContributingEvents() const {
    return contributingEvents_;
}

}  // namespace sentinelforge
