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

    const std::optional<Rule> rule = LoadRule();
    if (!rule.has_value()) {
        return 0;
    }

    RunDetection(*event, *rule);
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

std::optional<Rule> Application::LoadRule() const {
    try {
        return ruleParser_.ParseFile(SAMPLE_RULE_PATH);
    } catch (const std::exception& e) {
        logger_.Error(std::string("Failed to load detection rule: ") + e.what());
        return std::nullopt;
    }
}

void Application::RunDetection(const Event& event, const Rule& rule) const {
    const DetectionResult result = detectionEngine_.Evaluate(event, rule);

    if (result.Matched()) {
        logger_.Warning("Rule matched");
        logger_.Warning("  Rule: " + result.RuleName());
        logger_.Warning("  Severity: " + result.Severity());
        logger_.Warning("  MITRE: " + result.Mitre());
        logger_.Warning("  Reason: " + result.Reason());
    } else {
        logger_.Info("No detection matched.");
    }
}

}  // namespace sentinelforge
