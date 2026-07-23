#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

#include "DetectionReport.h"
#include "Logger.h"

namespace sentinelforge {

// Runtime settings for the JSON detection export. Owned by Configuration and
// passed to JsonExporter; the exporter never reads Configuration itself.
struct JsonExportSettings {
    bool enabled = true;
    std::filesystem::path outputFile{"detections.json"};
};

// Serializes a DetectionReport to a structured JSON document and optionally
// writes it to disk. Console reporting remains ReportPrinter's responsibility;
// this class is an additional output format only.
class JsonExporter {
public:
    // Builds the JSON document for `report`. Matched detections appear in the
    // detections array; if none matched, the array is empty (never omitted).
    nlohmann::json BuildDocument(const DetectionReport& report) const;

    // When settings.enabled is false, returns true immediately and writes
    // nothing. Otherwise writes BuildDocument(report) to settings.outputFile,
    // creating parent directories as needed. Failures are logged and return false;
    // success returns true. Does not throw.
    bool Export(const DetectionReport& report,
                const JsonExportSettings& settings,
                const Logger& logger) const;
};

}  // namespace sentinelforge
