#include "Application.h"

#include <stdexcept>
#include <string>
#include <utility>

#include "DetectionReport.h"

// Absolute path to the configuration file, resolved by CMake at configure
// time so the collector can locate it regardless of the working directory it
// is launched from. This is the only baked-in path: everything else is read
// from the file (or its documented defaults).
#ifndef CONFIG_FILE_PATH
#define CONFIG_FILE_PATH "config/sentinelforge.json"
#endif

namespace sentinelforge {

Application::Application() {
    PrintBanner();
}

int Application::Run() {
    const std::optional<Configuration> config = LoadConfiguration();
    if (!config.has_value()) {
        // Invalid configuration: the problem has already been reported.
        // Exit cleanly with a non-zero status rather than run with bad settings.
        return 1;
    }

    logger_.SetMinLevel(config->LoggingLevel());
    LogConfiguration(*config);

    const std::optional<Event> event = LoadEvent(config->SampleEventFile());
    if (!event.has_value()) {
        return 0;
    }

    const std::optional<RuleLoadResult> loadResult = LoadRules(config->RulesDirectory());
    if (!loadResult.has_value()) {
        return 0;
    }

    LogRuleLoadResult(*loadResult);
    RunDetection(*event, loadResult->Accepted());
    return 0;
}

void Application::PrintBanner() const {
    logger_.Info("SentinelForge Collector started");
}

std::optional<Configuration> Application::LoadConfiguration() const {
    try {
        return Configuration::LoadFromFile(CONFIG_FILE_PATH, logger_);
    } catch (const ConfigurationError& e) {
        logger_.Error(std::string("Configuration error: ") + e.what());
        return std::nullopt;
    }
}

void Application::LogConfiguration(const Configuration& config) const {
    logger_.Debug("Configuration loaded:");
    logger_.Debug("  rules_directory   = " + config.RulesDirectory().string());
    logger_.Debug("  sample_event_file = " + config.SampleEventFile().string());
    logger_.Debug("  output_directory  = " + config.OutputDirectory().string());
    logger_.Debug("  api_port          = " + std::to_string(config.ApiPort()));
    logger_.Debug(std::string("  dashboard_enabled = ") +
                  (config.DashboardEnabled() ? "true" : "false"));
}

std::optional<Event> Application::LoadEvent(const std::filesystem::path& sampleEventFile) const {
    try {
        return eventParser_.ParseFile(sampleEventFile);
    } catch (const std::exception& e) {
        logger_.Error(std::string("Failed to load telemetry event: ") + e.what());
        return std::nullopt;
    }
}

std::optional<RuleLoadResult> Application::LoadRules(
    const std::filesystem::path& rulesDirectory) const {
    try {
        return ruleLoader_.LoadDirectory(rulesDirectory);
    } catch (const std::exception& e) {
        logger_.Error(std::string("Failed to load detection rules: ") + e.what());
        return std::nullopt;
    }
}

void Application::LogRuleLoadResult(const RuleLoadResult& result) const {
    logger_.Info("Rules discovered: " + std::to_string(result.Discovered()));
    logger_.Info("Rules accepted: " + std::to_string(result.Accepted().size()));
    logger_.Info("Rules rejected: " + std::to_string(result.Rejected().size()));

    for (const auto& rejection : result.Rejected()) {
        logger_.Error("ERROR:");
        logger_.Error("Rule \"" + rejection.Identifier() + "\"");
        for (const auto& message : rejection.Errors()) {
            logger_.Error(message);
        }
    }
}

void Application::RunDetection(const Event& event, const std::vector<Rule>& rules) const {
    std::vector<DetectionResult> results = detectionEngine_.Evaluate(event, rules);
    const std::size_t rulesEvaluated = results.size();
    const DetectionReport report(event, rules.size(), rulesEvaluated, std::move(results));

    reportPrinter_.Print(report, logger_);
}

}  // namespace sentinelforge
