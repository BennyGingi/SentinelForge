#pragma once

#include <vector>

#include "CorrelationAlert.h"
#include "DetectionReport.h"
#include "Logger.h"

namespace sentinelforge {

// Renders DetectionReport and CorrelationAlert output via Logger.
// Pure presentation: no matching or correlation logic.
class ReportPrinter {
public:
    void Print(const DetectionReport& report, const Logger& logger) const;
    void PrintCorrelationAlerts(const std::vector<CorrelationAlert>& alerts,
                                const Logger& logger) const;
};

}  // namespace sentinelforge
