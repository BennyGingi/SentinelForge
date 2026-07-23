#include "JsonExporter.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <system_error>

namespace sentinelforge {

namespace {

constexpr std::string_view kComponent = "JsonExporter";

std::string UtcTimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);

    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &seconds);
#else
    gmtime_r(&seconds, &utc);
#endif

    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

}  // namespace

nlohmann::json JsonExporter::BuildDocument(const DetectionReport& report) const {
    nlohmann::json detections = nlohmann::json::array();

    for (const auto& result : report.Results()) {
        if (!result.Matched()) {
            continue;
        }
        detections.push_back({{"rule", result.RuleName()},
                              {"severity", result.Severity()},
                              {"mitre", result.Mitre()},
                              {"process", report.ProcessName()},
                              {"reason", result.Reason()}});
    }

    return nlohmann::json{{"generated_at", UtcTimestampNow()},
                          {"rules_loaded", report.RulesLoaded()},
                          {"rules_evaluated", report.RulesEvaluated()},
                          {"matches", report.MatchesFound()},
                          {"detections", std::move(detections)}};
}

bool JsonExporter::Export(const DetectionReport& report,
                          const JsonExportSettings& settings,
                          const Logger& logger) const {
    if (!settings.enabled) {
        logger.Debug(kComponent, "JSON export disabled; skipping");
        return true;
    }

    if (settings.outputFile.empty()) {
        logger.Error(kComponent, "JSON export failed: output path is empty");
        return false;
    }

    logger.Info(kComponent, "JSON export started");
    logger.Info(kComponent, "Output file: " + settings.outputFile.string());

    const nlohmann::json document = BuildDocument(report);

    const std::filesystem::path parent = settings.outputFile.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            logger.Error(kComponent,
                         "JSON export failed: cannot create directory '" + parent.string() +
                             "': " + ec.message());
            return false;
        }
    }

    std::ofstream stream(settings.outputFile);
    if (!stream.is_open()) {
        logger.Error(kComponent,
                     "JSON export failed: cannot open '" + settings.outputFile.string() +
                         "' for writing");
        return false;
    }

    try {
        stream << document.dump(2) << '\n';
    } catch (const std::exception& e) {
        logger.Error(kComponent, std::string("JSON export failed: ") + e.what());
        return false;
    }

    if (!stream.good()) {
        logger.Error(kComponent,
                     "JSON export failed: write error for '" + settings.outputFile.string() + "'");
        return false;
    }

    logger.Info(kComponent, "JSON export completed");
    return true;
}

}  // namespace sentinelforge
