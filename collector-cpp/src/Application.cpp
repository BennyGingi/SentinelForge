#include "Application.h"

#include <stdexcept>
#include <utility>

#include "DetectionReport.h"

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
        return eventParser_.ParseFile(SAMPLE_LOG_PATH);
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
    std::vector<DetectionResult> results = detectionEngine_.Evaluate(event, rules);
    const std::size_t rulesEvaluated = results.size();
    const DetectionReport report(event, rules.size(), rulesEvaluated, std::move(results));

    reportPrinter_.Print(report, logger_);
}

}  // namespace sentinelforge
