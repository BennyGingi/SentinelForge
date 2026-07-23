#include "EventMonitor.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include "DetectionReport.h"

namespace sentinelforge {

namespace {

constexpr std::string_view kComponent = "EventMonitor";

}  // namespace

EventMonitor::EventMonitor(MonitoringSettings settings,
                           JsonExportSettings jsonExport,
                           std::vector<Rule> rules,
                           EventParser& eventParser,
                           DetectionEngine& detectionEngine,
                           ReportPrinter& reportPrinter,
                           JsonExporter& jsonExporter,
                           PerformanceProfiler& profiler,
                           Logger& logger)
    : settings_(std::move(settings)),
      jsonExport_(std::move(jsonExport)),
      rules_(std::move(rules)),
      eventParser_(eventParser),
      detectionEngine_(detectionEngine),
      reportPrinter_(reportPrinter),
      jsonExporter_(jsonExporter),
      profiler_(profiler),
      logger_(logger) {}

void EventMonitor::RequestStop() { stopRequested_.store(true); }

bool EventMonitor::StopRequested() const { return stopRequested_.load(); }

int EventMonitor::Run() {
    EnsureDirectories();

    logger_.Info(kComponent, "Monitoring started");
    logger_.Info(kComponent, "Input directory: " + settings_.inputDirectory.string());
    logger_.Info(kComponent, "Processed directory: " + settings_.processedDirectory.string());

    while (!StopRequested()) {
        logger_.Info(kComponent, "Waiting for events");

        const std::vector<std::filesystem::path> pending = ListIncomingEvents();
        if (pending.empty()) {
            InterruptibleWait();
            continue;
        }

        for (const auto& path : pending) {
            if (StopRequested()) {
                break;
            }
            // Finish the current event even if stop arrives mid-processing.
            ProcessEventFile(path);
        }
    }

    logger_.Info(kComponent, "Shutdown");
    return 0;
}

void EventMonitor::EnsureDirectories() const {
    std::error_code ec;
    std::filesystem::create_directories(settings_.inputDirectory, ec);
    if (ec) {
        throw std::runtime_error("Failed to create input directory '" +
                                 settings_.inputDirectory.string() + "': " + ec.message());
    }
    std::filesystem::create_directories(settings_.processedDirectory, ec);
    if (ec) {
        throw std::runtime_error("Failed to create processed directory '" +
                                 settings_.processedDirectory.string() + "': " + ec.message());
    }
}

std::vector<std::filesystem::path> EventMonitor::ListIncomingEvents() const {
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::is_directory(settings_.inputDirectory)) {
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(settings_.inputDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

void EventMonitor::ProcessEventFile(const std::filesystem::path& path) {
    logger_.Info(kComponent, "Event detected: " + path.filename().string());
    logger_.Info(kComponent, "Processing started");

    profiler_.Clear();
    profiler_.Start(ProfileStage::TotalProcessing);

    std::optional<Event> event;
    profiler_.Start(ProfileStage::ParseTime);
    try {
        event = eventParser_.ParseFile(path);
    } catch (const std::exception& e) {
        profiler_.Stop(ProfileStage::ParseTime);
        profiler_.Stop(ProfileStage::TotalProcessing);
        logger_.Error(kComponent, std::string("Failed to parse event: ") + e.what());
        ArchiveProcessedFile(path);
        profiler_.LogSummary(logger_);
        return;
    }
    profiler_.Stop(ProfileStage::ParseTime);

    profiler_.Start(ProfileStage::DetectionTime);
    std::vector<DetectionResult> results = detectionEngine_.Evaluate(*event, rules_);
    profiler_.Stop(ProfileStage::DetectionTime);
    logger_.Info(kComponent, "Detection completed");

    const DetectionReport report(*event, rules_.size(), results.size(), std::move(results));

    profiler_.Start(ProfileStage::ReportGenerationTime);
    reportPrinter_.Print(report, logger_);
    profiler_.Stop(ProfileStage::ReportGenerationTime);
    logger_.Info(kComponent, "Report generated");

    profiler_.Start(ProfileStage::JsonExportTime);
    jsonExporter_.Export(report, jsonExport_, logger_);
    profiler_.Stop(ProfileStage::JsonExportTime);
    logger_.Info(kComponent, "JSON exported");

    ArchiveProcessedFile(path);

    profiler_.Stop(ProfileStage::TotalProcessing);
    profiler_.LogSummary(logger_);
}

void EventMonitor::ArchiveProcessedFile(const std::filesystem::path& path) {
    const std::filesystem::path destination = settings_.processedDirectory / path.filename();
    std::filesystem::path finalDestination = destination;
    if (std::filesystem::exists(finalDestination)) {
        const auto stem = path.stem().string();
        const auto ext = path.extension().string();
        const auto stamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        finalDestination =
            settings_.processedDirectory / (stem + "_" + std::to_string(stamp) + ext);
    }

    std::error_code ec;
    std::filesystem::rename(path, finalDestination, ec);
    if (ec) {
        // Cross-device rename can fail; fall back to copy + remove.
        std::filesystem::copy_file(path, finalDestination,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            std::filesystem::remove(path, ec);
        }
    }

    if (ec) {
        logger_.Error(kComponent, "Failed to archive '" + path.string() + "': " + ec.message());
        return;
    }

    logger_.Info(kComponent, "File archived: " + finalDestination.filename().string());
}

void EventMonitor::InterruptibleWait() {
    const auto interval = std::chrono::milliseconds{settings_.pollIntervalMs};
    const auto slice = std::chrono::milliseconds{50};
    auto remaining = interval;
    while (remaining.count() > 0 && !StopRequested()) {
        const auto step = remaining < slice ? remaining : slice;
        std::this_thread::sleep_for(step);
        remaining -= step;
    }
}

}  // namespace sentinelforge
