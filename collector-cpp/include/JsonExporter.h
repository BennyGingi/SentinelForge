#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "CorrelationAlert.h"
#include "DetectionReport.h"
#include "Logger.h"

namespace sentinelforge {

// Runtime settings for the JSON detection export. Owned by Configuration and
// passed to JsonExporter; the exporter never reads Configuration itself.
struct JsonExportSettings {
    bool enabled = true;
    std::filesystem::path outputFile{"detections.json"};
};

// Serializes a DetectionReport (and optional correlation alerts) to a
// structured JSON document and optionally writes it to disk. Console reporting
// remains ReportPrinter's responsibility; this class is an additional output.
class JsonExporter {
public:
    // Builds the JSON document for `report`. Matched detections appear in the
    // detections array; correlation_alerts is always present (possibly empty).
    nlohmann::json BuildDocument(const DetectionReport& report,
                                 const std::vector<CorrelationAlert>& alerts = {}) const;

    // When settings.enabled is false, returns true immediately and writes
    // nothing. Otherwise writes BuildDocument(...) to settings.outputFile,
    // creating parent directories as needed. Failures are logged and return false;
    // success returns true. Does not throw.
    bool Export(const DetectionReport& report,
                const JsonExportSettings& settings,
                const Logger& logger,
                const std::vector<CorrelationAlert>& alerts = {}) const;
};

}  // namespace sentinelforge
