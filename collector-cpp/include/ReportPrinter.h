#pragma once

#include "DetectionReport.h"
#include "Logger.h"

namespace sentinelforge {

// Renders a DetectionReport to the console via Logger. Pure presentation:
// no matching logic, no parsing, no data ownership beyond the report itself.
class ReportPrinter {
public:
    void Print(const DetectionReport& report, const Logger& logger) const;
};

}  // namespace sentinelforge
