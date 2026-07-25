#include "ApiServer.h"

#include <chrono>
#include <cstdlib>
#include <string_view>

#include <httplib.h>
#include <nlohmann/json.hpp>

#ifndef COLLECTOR_VERSION
#define COLLECTOR_VERSION "0.0.0"
#endif

namespace sentinelforge {

namespace {

constexpr std::string_view kComponent = "ApiServer";

std::uint64_t ParseSince(const httplib::Request& req) {
    if (!req.has_param("since")) {
        return 0;
    }
    const std::string raw = req.get_param_value("since");
    char* end = nullptr;
    const unsigned long long value = std::strtoull(raw.c_str(), &end, 10);
    if (end == raw.c_str()) {
        return 0;  // unparsable -> treat as "from the beginning", never 500
    }
    return static_cast<std::uint64_t>(value);
}

nlohmann::json ToJson(const StoredDetection& d) {
    return nlohmann::json{{"sequence", d.sequence},
                          {"id", d.id},
                          {"timestamp_ms", d.timestampMs},
                          {"severity", d.severity},
                          {"rule_name", d.ruleName},
                          {"mitre", d.mitre},
                          {"process_name", d.processName},
                          {"parent_process", d.parentProcess},
                          {"command_line", d.commandLine},
                          {"host", d.host},
                          {"user", d.user},
                          {"reason", d.reason},
                          {"pid", d.pid},
                          {"parent_pid", d.parentPid}};
}

nlohmann::json ToJson(const StoredCorrelationAlert& a) {
    return nlohmann::json{{"sequence", a.sequence},
                          {"id", a.id},
                          {"timestamp_ms", a.timestampMs},
                          {"title", a.title},
                          {"description", a.description},
                          {"severity", a.severity},
                          {"confidence", a.confidence},
                          {"matched_event_count", a.matchedEventCount},
                          {"mitre_techniques", a.mitreTechniques}};
}

nlohmann::json ToJson(const StoredLogLine& l) {
    return nlohmann::json{{"sequence", l.sequence},
                          {"timestamp_ms", l.timestampMs},
                          {"level", l.level},
                          {"component", l.component},
                          {"message", l.message}};
}

template <typename T>
nlohmann::json PageToJson(const CursorPage<T>& page) {
    nlohmann::json items = nlohmann::json::array();
    for (const auto& item : page.items) {
        items.push_back(ToJson(item));
    }
    return nlohmann::json{
        {"cursor", page.cursor}, {"more", page.more}, {"items", std::move(items)}};
}

}  // namespace

ApiServer::ApiServer(TelemetryStore& store, ApiSettings settings, Logger& logger)
    : store_(store),
      settings_(std::move(settings)),
      logger_(logger),
      server_(std::make_unique<httplib::Server>()) {
    RegisterRoutes();
}

ApiServer::~ApiServer() { Stop(); }

void ApiServer::RegisterRoutes() {
    server_->Get("/health", [this](const httplib::Request&, httplib::Response& res) {
        const auto uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::steady_clock::now() - store_.StartTime())
                                       .count();
        const nlohmann::json body{
            {"status", "ok"}, {"uptime_seconds", uptimeSeconds}, {"version", COLLECTOR_VERSION}};
        res.set_content(body.dump(), "application/json");
    });

    server_->Get("/stats", [this](const httplib::Request&, httplib::Response& res) {
        const TelemetryStats stats = store_.Stats();
        const nlohmann::json body{{"rules_loaded", stats.rulesLoaded},
                                  {"sigma_rules_loaded", stats.sigmaRulesLoaded},
                                  {"correlation_rules_loaded", stats.correlationRulesLoaded},
                                  {"events_processed", stats.eventsProcessed},
                                  {"detections", stats.detections},
                                  {"correlation_alerts", stats.correlationAlerts},
                                  {"events_per_second", stats.eventsPerSecond},
                                  {"pipeline_latency_ms", stats.pipelineLatencyMs}};
        res.set_content(body.dump(), "application/json");
    });

    server_->Get("/detections", [this](const httplib::Request& req, httplib::Response& res) {
        const auto page = store_.DetectionsSince(ParseSince(req));
        res.set_content(PageToJson(page).dump(), "application/json");
    });

    server_->Get("/correlations", [this](const httplib::Request& req, httplib::Response& res) {
        const auto page = store_.AlertsSince(ParseSince(req));
        res.set_content(PageToJson(page).dump(), "application/json");
    });

    server_->Get("/logs", [this](const httplib::Request& req, httplib::Response& res) {
        const auto page = store_.LogsSince(ParseSince(req));
        res.set_content(PageToJson(page).dump(), "application/json");
    });
}

void ApiServer::Start() {
    if (!settings_.enabled || running_.load()) {
        return;
    }
    running_.store(true);
    thread_ = std::thread(&ApiServer::Run, this);
}

void ApiServer::Run() {
    logger_.Info(kComponent,
                 "Listening on " + settings_.bindAddress + ":" + std::to_string(settings_.port));
    if (!server_->listen(settings_.bindAddress, settings_.port)) {
        logger_.Error(kComponent, "Failed to bind " + settings_.bindAddress + ":" +
                                      std::to_string(settings_.port));
    }
}

void ApiServer::Stop() {
    if (!running_.load()) {
        return;
    }
    server_->stop();
    if (thread_.joinable()) {
        thread_.join();
    }
    running_.store(false);
    logger_.Info(kComponent, "Stopped");
}

}  // namespace sentinelforge
