#include "PerformanceProfiler.h"

#include <iomanip>
#include <sstream>

namespace sentinelforge {

namespace {

constexpr std::string_view kComponent = "PerformanceProfiler";

bool IsTotalStage(std::string_view name) {
    return name == ProfileStage::TotalRuntime || name == ProfileStage::TotalProcessing;
}

}  // namespace

void PerformanceProfiler::Start(std::string_view stage) {
    const std::string key(stage);
    openTimers_[key] = Clock::now();
}

void PerformanceProfiler::Stop(std::string_view stage) {
    const std::string key(stage);
    const auto it = openTimers_.find(key);
    if (it == openTimers_.end()) {
        // Unknown / already-stopped stage: safe no-op.
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() -
                                                                               it->second);
    openTimers_.erase(it);

    const auto existing = measurementIndex_.find(key);
    if (existing != measurementIndex_.end()) {
        measurements_[existing->second].elapsed = elapsed;
        return;
    }

    measurementIndex_[key] = measurements_.size();
    measurements_.push_back(TimingMeasurement{key, elapsed});
}

void PerformanceProfiler::Clear() {
    openTimers_.clear();
    measurements_.clear();
    measurementIndex_.clear();
}

bool PerformanceProfiler::Has(std::string_view stage) const {
    return measurementIndex_.find(std::string(stage)) != measurementIndex_.end();
}

std::chrono::milliseconds PerformanceProfiler::Elapsed(std::string_view stage) const {
    const auto it = measurementIndex_.find(std::string(stage));
    if (it == measurementIndex_.end()) {
        return std::chrono::milliseconds{0};
    }
    return measurements_[it->second].elapsed;
}

const std::vector<TimingMeasurement>& PerformanceProfiler::Measurements() const {
    return measurements_;
}

std::chrono::milliseconds PerformanceProfiler::SumOfStages() const {
    std::chrono::milliseconds total{0};
    for (const auto& sample : measurements_) {
        if (!IsTotalStage(sample.name)) {
            total += sample.elapsed;
        }
    }
    return total;
}

std::string PerformanceProfiler::FormatSummary() const {
    std::ostringstream out;
    out << "==============================\n";
    out << "Performance Summary\n";
    out << "==============================\n";
    out << "\n";

    constexpr int kLabelWidth = 22;
    for (const auto& sample : measurements_) {
        if (IsTotalStage(sample.name)) {
            continue;
        }
        out << std::left << std::setw(kLabelWidth) << sample.name << ": "
            << sample.elapsed.count() << " ms\n";
    }

    out << "\n";
    out << "--------------------------------\n";
    out << "\n";

    const auto total = Has(ProfileStage::TotalProcessing) ? Elapsed(ProfileStage::TotalProcessing)
                       : Has(ProfileStage::TotalRuntime)  ? Elapsed(ProfileStage::TotalRuntime)
                                                          : SumOfStages();
    const char* totalLabel =
        Has(ProfileStage::TotalProcessing) ? "Total processing time" : "Total Runtime";
    out << std::left << std::setw(kLabelWidth) << totalLabel << ": " << total.count() << " ms\n";

    return out.str();
}

void PerformanceProfiler::LogSummary(const Logger& logger) const {
    const std::string summary = FormatSummary();
    std::istringstream stream(summary);
    std::string line;
    while (std::getline(stream, line)) {
        logger.Info(kComponent, line);
    }
}

}  // namespace sentinelforge
