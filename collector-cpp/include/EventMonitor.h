#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "DetectionEngine.h"
#include "EventParser.h"
#include "JsonExporter.h"
#include "Logger.h"
#include "PerformanceProfiler.h"
#include "ReportPrinter.h"
#include "Rule.h"

namespace sentinelforge {

// Runtime settings for continuous file-based event monitoring.
struct MonitoringSettings {
    bool enabled = true;
    std::filesystem::path inputDirectory{"events/incoming"};
    std::filesystem::path processedDirectory{"events/processed"};
    std::uint32_t pollIntervalMs = 1000;
};

// Continuously polls an incoming directory for telemetry JSON files, runs the
// existing detection/report/export pipeline for each file, then archives the
// file under the processed directory. DetectionEngine is used as a pure
// evaluator and is never modified.
//
// Call RequestStop() (e.g. from a CTRL+C handler) to finish the current event
// and exit the monitoring loop cleanly.
class EventMonitor {
public:
    EventMonitor(MonitoringSettings settings,
                 JsonExportSettings jsonExport,
                 std::vector<Rule> rules,
                 EventParser& eventParser,
                 DetectionEngine& detectionEngine,
                 ReportPrinter& reportPrinter,
                 JsonExporter& jsonExporter,
                 PerformanceProfiler& profiler,
                 Logger& logger);

    // Blocks until RequestStop() is observed after the current event (if any)
    // completes. Returns 0 on clean shutdown.
    int Run();

    // Signals the monitor to stop after finishing the in-flight event.
    void RequestStop();

    bool StopRequested() const;

private:
    void EnsureDirectories() const;
    std::vector<std::filesystem::path> ListIncomingEvents() const;
    void ProcessEventFile(const std::filesystem::path& path);
    void ArchiveProcessedFile(const std::filesystem::path& path);
    void InterruptibleWait();

    MonitoringSettings settings_;
    JsonExportSettings jsonExport_;
    std::vector<Rule> rules_;
    EventParser& eventParser_;
    DetectionEngine& detectionEngine_;
    ReportPrinter& reportPrinter_;
    JsonExporter& jsonExporter_;
    PerformanceProfiler& profiler_;
    Logger& logger_;
    std::atomic<bool> stopRequested_{false};
};

}  // namespace sentinelforge
