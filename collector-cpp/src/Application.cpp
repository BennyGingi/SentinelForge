#include "Application.h"

#include <stdexcept>
#include <string>
#include <unordered_set>
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
    profiler_.Start(ProfileStage::TotalRuntime);

    int exitCode = 0;

    const std::optional<Configuration> config = LoadConfiguration();
    if (!config.has_value()) {
        // Invalid configuration: the problem has already been reported.
        // Exit cleanly with a non-zero status rather than run with bad settings.
        exitCode = 1;
    } else {
        // Apply configured destinations and level. Logger reads LoggingSettings
        // only — it never depends on Configuration itself.
        logger_.Configure(config->Logging());
        LogConfiguration(*config);

        const std::optional<Event> event = LoadEvent(config->SampleEventFile());
        if (!event.has_value()) {
            exitCode = 0;
        } else {
            const std::optional<RuleLoadResult> loadResult = LoadRules(*config);
            if (!loadResult.has_value()) {
                exitCode = 0;
            } else {
                LogRuleLoadResult(*loadResult);
                RunDetection(*event, loadResult->Accepted(), config->JsonExport());
                exitCode = 0;
            }
        }
    }

    profiler_.Stop(ProfileStage::TotalRuntime);
    PrintPerformanceSummary();
    return exitCode;
}

void Application::PrintBanner() const {
    logger_.Info("Application", "SentinelForge Collector started");
}

std::optional<Configuration> Application::LoadConfiguration() {
    profiler_.Start(ProfileStage::Configuration);
    std::optional<Configuration> result;
    try {
        result = Configuration::LoadFromFile(CONFIG_FILE_PATH, logger_);
    } catch (const ConfigurationError& e) {
        logger_.Error("Configuration", std::string("Configuration error: ") + e.what());
        result = std::nullopt;
    }
    profiler_.Stop(ProfileStage::Configuration);
    return result;
}

void Application::LogConfiguration(const Configuration& config) const {
    if (!logger_.ShouldLog(LogLevel::Debug)) {
        return;
    }

    auto levelName = [](LogLevel level) -> const char* {
        switch (level) {
            case LogLevel::Trace:
                return "TRACE";
            case LogLevel::Debug:
                return "DEBUG";
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warn:
                return "WARN";
            case LogLevel::Error:
                return "ERROR";
            case LogLevel::Fatal:
                return "FATAL";
        }
        return "INFO";
    };

    logger_.Debug("Configuration", "Configuration loaded:");
    logger_.Debug("Configuration", "  rules_directory   = " + config.RulesDirectory().string());
    logger_.Debug("Configuration",
                  "  sample_event_file = " + config.SampleEventFile().string());
    logger_.Debug("Configuration", "  output_directory  = " + config.OutputDirectory().string());
    logger_.Debug("Configuration", "  api_port          = " + std::to_string(config.ApiPort()));
    logger_.Debug("Configuration",
                  std::string("  dashboard_enabled = ") +
                      (config.DashboardEnabled() ? "true" : "false"));
    logger_.Debug("Configuration",
                  std::string("  logging.level     = ") + levelName(config.Logging().level));
    logger_.Debug("Configuration",
                  std::string("  json_export.enabled = ") +
                      (config.JsonExport().enabled ? "true" : "false"));
    logger_.Debug("Configuration",
                  "  json_export.output_file = " + config.JsonExport().outputFile.string());
    logger_.Debug("Configuration",
                  std::string("  sigma.enabled = ") + (config.Sigma().enabled ? "true" : "false"));
    logger_.Debug("Configuration",
                  "  sigma.rules_directory = " + config.Sigma().rulesDirectory.string());
}

std::optional<Event> Application::LoadEvent(const std::filesystem::path& sampleEventFile) {
    profiler_.Start(ProfileStage::EventLoading);
    std::optional<Event> result;
    try {
        result = eventParser_.ParseFile(sampleEventFile);
    } catch (const std::exception& e) {
        logger_.Error("Application", std::string("Failed to load telemetry event: ") + e.what());
        result = std::nullopt;
    }
    profiler_.Stop(ProfileStage::EventLoading);
    return result;
}

std::optional<RuleLoadResult> Application::LoadRules(const Configuration& config) {
    try {
        profiler_.Start(ProfileStage::RuleLoading);
        const std::vector<DiscoveredRule> discovered =
            ruleLoader_.DiscoverAndParse(config.RulesDirectory());
        profiler_.Stop(ProfileStage::RuleLoading);

        profiler_.Start(ProfileStage::RuleValidation);
        RuleLoadResult native = ruleLoader_.ValidateRules(discovered);

        std::vector<Rule> accepted = native.Accepted();
        std::vector<RejectedRule> rejected = native.Rejected();

        if (config.Sigma().enabled) {
            std::unordered_set<std::string> existingNames;
            for (const auto& rule : accepted) {
                existingNames.insert(rule.RuleName());
            }

            const RuleLoadResult sigma =
                sigmaLoader_.LoadDirectory(config.Sigma().rulesDirectory, existingNames, logger_);
            for (const auto& rule : sigma.Accepted()) {
                accepted.push_back(rule);
            }
            for (const auto& item : sigma.Rejected()) {
                rejected.push_back(item);
            }
        } else {
            logger_.Info("SigmaLoader", "Sigma loading disabled; skipping");
        }

        profiler_.Stop(ProfileStage::RuleValidation);
        return RuleLoadResult(std::move(accepted), std::move(rejected));
    } catch (const std::exception& e) {
        // Ensure open stage timers are closed even when discovery throws.
        profiler_.Stop(ProfileStage::RuleLoading);
        profiler_.Stop(ProfileStage::RuleValidation);
        logger_.Error("RuleLoader", std::string("Failed to load detection rules: ") + e.what());
        return std::nullopt;
    }
}

void Application::LogRuleLoadResult(const RuleLoadResult& result) const {
    logger_.Info("RuleLoader", "Rules discovered: " + std::to_string(result.Discovered()));
    logger_.Info("RuleLoader", "Rules accepted: " + std::to_string(result.Accepted().size()));
    logger_.Info("RuleLoader", "Rules rejected: " + std::to_string(result.Rejected().size()));

    for (const auto& rejection : result.Rejected()) {
        logger_.Error("RuleValidator", "ERROR:");
        logger_.Error("RuleValidator", "Rule \"" + rejection.Identifier() + "\"");
        for (const auto& message : rejection.Errors()) {
            logger_.Error("RuleValidator", message);
        }
    }
}

void Application::RunDetection(const Event& event,
                               const std::vector<Rule>& rules,
                               const JsonExportSettings& jsonExport) {
    profiler_.Start(ProfileStage::DetectionEngine);
    std::vector<DetectionResult> results = detectionEngine_.Evaluate(event, rules);
    profiler_.Stop(ProfileStage::DetectionEngine);

    const std::size_t rulesEvaluated = results.size();
    const DetectionReport report(event, rules.size(), rulesEvaluated, std::move(results));

    profiler_.Start(ProfileStage::ReportGeneration);
    reportPrinter_.Print(report, logger_);
    jsonExporter_.Export(report, jsonExport, logger_);
    profiler_.Stop(ProfileStage::ReportGeneration);
}

void Application::PrintPerformanceSummary() const {
    profiler_.LogSummary(logger_);
}

}  // namespace sentinelforge
