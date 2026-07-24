#include "ReportPrinter.h"

#include <cstddef>
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

void ReportPrinter::PrintCorrelationAlerts(const std::vector<CorrelationAlert>& alerts,
                                           const Logger& logger) const {
    if (alerts.empty()) {
        return;
    }

    for (const auto& alert : alerts) {
        logger.Warn("CorrelationEngine", "Correlation alert");
        logger.Warn("CorrelationEngine", "  Title: " + alert.Title());
        logger.Warn("CorrelationEngine", "  Severity: " + alert.Severity());
        logger.Warn("CorrelationEngine",
                    "  Confidence: " + std::to_string(alert.Confidence()));
        logger.Warn("CorrelationEngine", "  Timestamp: " + alert.Timestamp());
        logger.Warn("CorrelationEngine",
                    "  Matched events: " + std::to_string(alert.MatchedEventCount()));
        logger.Warn("CorrelationEngine", "  Description: " + alert.Description());
        if (!alert.MitreTechniques().empty()) {
            std::string mitre;
            for (std::size_t i = 0; i < alert.MitreTechniques().size(); ++i) {
                if (i > 0) {
                    mitre += ", ";
                }
                mitre += alert.MitreTechniques()[i];
            }
            logger.Warn("CorrelationEngine", "  MITRE: " + mitre);
        }
    }
}

}  // namespace sentinelforge
