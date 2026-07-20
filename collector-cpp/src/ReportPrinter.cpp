#include "ReportPrinter.h"

#include <string>

namespace sentinelforge {

void ReportPrinter::Print(const DetectionReport& report, const Logger& logger) const {
    logger.Info("Loaded event:");
    logger.Info("  Timestamp: " + report.Timestamp());
    logger.Info("  Hostname: " + report.Hostname());
    logger.Info("  User: " + report.Username());
    logger.Info("  Process: " + report.ProcessName());
    logger.Info("  CommandLine: " + report.CommandLine());

    for (const auto& result : report.Results()) {
        if (!result.Matched()) {
            continue;
        }

        logger.Warning("Rule matched");
        logger.Warning("  Rule: " + result.RuleName());
        logger.Warning("  Severity: " + result.Severity());
        logger.Warning("  MITRE: " + result.Mitre());
        logger.Warning("  Reason: " + result.Reason());
    }

    if (report.MatchesFound() == 0) {
        logger.Info("No detections matched.");
    }

    logger.Info("Rules loaded: " + std::to_string(report.RulesLoaded()));
    logger.Info("Rules evaluated: " + std::to_string(report.RulesEvaluated()));
    logger.Info("Matches: " + std::to_string(report.MatchesFound()));
}

}  // namespace sentinelforge
