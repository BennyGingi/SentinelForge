#include "ReportPrinter.h"

#include <string>

namespace sentinelforge {

void ReportPrinter::Print(const DetectionReport& report, const Logger& logger) const {
    logger.Info("ReportPrinter", "Loaded event:");
    logger.Info("ReportPrinter", "  Timestamp: " + report.Timestamp());
    logger.Info("ReportPrinter", "  Hostname: " + report.Hostname());
    logger.Info("ReportPrinter", "  User: " + report.Username());
    logger.Info("ReportPrinter", "  Process: " + report.ProcessName());
    logger.Info("ReportPrinter", "  CommandLine: " + report.CommandLine());

    for (const auto& result : report.Results()) {
        if (!result.Matched()) {
            continue;
        }

        logger.Warn("DetectionEngine", "Rule matched");
        logger.Warn("DetectionEngine", "  Rule: " + result.RuleName());
        logger.Warn("DetectionEngine", "  Severity: " + result.Severity());
        logger.Warn("DetectionEngine", "  MITRE: " + result.Mitre());
        logger.Warn("DetectionEngine", "  Reason: " + result.Reason());
    }

    if (report.MatchesFound() == 0) {
        logger.Info("ReportPrinter", "No detections matched.");
    }

    logger.Info("ReportPrinter", "Rules loaded: " + std::to_string(report.RulesLoaded()));
    logger.Info("ReportPrinter", "Rules evaluated: " + std::to_string(report.RulesEvaluated()));
    logger.Info("ReportPrinter", "Matches: " + std::to_string(report.MatchesFound()));
}

}  // namespace sentinelforge
