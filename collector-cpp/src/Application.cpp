#include "Application.h"

#include <stdexcept>

namespace sentinelforge {

Application::Application() {
    PrintBanner();
}

int Application::Run() {
    const std::optional<Event> event = LoadEvent();
    if (!event.has_value()) {
        return 0;
    }

    const std::optional<std::vector<Rule>> rules = LoadRules();
    if (!rules.has_value()) {
        return 0;
    }

    RunDetection(*event, *rules);
    return 0;
}

void Application::PrintBanner() const {
    logger_.Info("SentinelForge Collector started");
}

std::optional<Event> Application::LoadEvent() const {
    try {
        const Event event = eventParser_.ParseFile(SAMPLE_LOG_PATH);

        logger_.Info("Loaded event:");
        logger_.Info("  Timestamp: " + event.Timestamp());
        logger_.Info("  Hostname: " + event.Hostname());
        logger_.Info("  User: " + event.Username());
        logger_.Info("  Process: " + event.ProcessName());
        logger_.Info("  CommandLine: " + event.CommandLine());

        return event;
    } catch (const std::exception& e) {
        logger_.Error(std::string("Failed to load telemetry event: ") + e.what());
        return std::nullopt;
    }
}

std::optional<std::vector<Rule>> Application::LoadRules() const {
    try {
        return ruleLoader_.LoadDirectory(RULES_DIR);
    } catch (const std::exception& e) {
        logger_.Error(std::string("Failed to load detection rules: ") + e.what());
        return std::nullopt;
    }
}

void Application::RunDetection(const Event& event, const std::vector<Rule>& rules) const {
    const std::vector<DetectionResult> results = detectionEngine_.Evaluate(event, rules);

    std::size_t matches = 0;
    for (const auto& result : results) {
        if (result.Matched()) {
            ++matches;
            logger_.Warning("Rule matched");
            logger_.Warning("  Rule: " + result.RuleName());
            logger_.Warning("  Severity: " + result.Severity());
            logger_.Warning("  MITRE: " + result.Mitre());
            logger_.Warning("  Reason: " + result.Reason());
        }
    }

    if (matches == 0) {
        logger_.Info("No detections matched.");
    }

    logger_.Info("Rules loaded: " + std::to_string(rules.size()));
    logger_.Info("Rules evaluated: " + std::to_string(results.size()));
    logger_.Info("Matches: " + std::to_string(matches));
}

}  // namespace sentinelforge
