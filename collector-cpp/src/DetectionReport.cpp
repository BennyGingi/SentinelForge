#include "DetectionReport.h"

#include <algorithm>
#include <utility>

namespace sentinelforge {

namespace {

std::size_t CountMatches(const std::vector<DetectionResult>& results) {
    return static_cast<std::size_t>(
        std::count_if(results.begin(), results.end(),
                      [](const DetectionResult& result) { return result.Matched(); }));
}

}  // namespace

DetectionReport::DetectionReport(const Event& event,
                                  std::size_t rulesLoaded,
                                  std::size_t rulesEvaluated,
                                  std::vector<DetectionResult> results)
    : timestamp_(event.Timestamp()),
      hostname_(event.Hostname()),
      username_(event.Username()),
      processName_(event.ProcessName()),
      commandLine_(event.CommandLine()),
      rulesLoaded_(rulesLoaded),
      rulesEvaluated_(rulesEvaluated),
      matchesFound_(CountMatches(results)),
      results_(std::move(results)) {}

const std::string& DetectionReport::Timestamp() const { return timestamp_; }
const std::string& DetectionReport::Hostname() const { return hostname_; }
const std::string& DetectionReport::Username() const { return username_; }
const std::string& DetectionReport::ProcessName() const { return processName_; }
const std::string& DetectionReport::CommandLine() const { return commandLine_; }
std::size_t DetectionReport::RulesLoaded() const { return rulesLoaded_; }
std::size_t DetectionReport::RulesEvaluated() const { return rulesEvaluated_; }
std::size_t DetectionReport::MatchesFound() const { return matchesFound_; }
const std::vector<DetectionResult>& DetectionReport::Results() const { return results_; }

}  // namespace sentinelforge
