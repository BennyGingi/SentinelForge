#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "Logger.h"
#include "TelemetryStore.h"

// Forward-declared rather than #include <httplib.h> here: httplib::Server
// is only ever touched inside ApiServer.cpp, so callers of this header
// (Application.h) don't need to compile against it.
namespace httplib {
class Server;
}

namespace sentinelforge {

// Runtime settings for the collector's read-only HTTP API (Issue #035).
struct ApiSettings {
    bool enabled = true;
    std::string bindAddress{"127.0.0.1"};
    std::uint16_t port = 8787;
};

// Owns an httplib::Server on its own thread, serving TelemetryStore contents
// as JSON. No mutation endpoints -- the API is read-only by design, matching
// the GUI's own "no widget parses/mutates telemetry" contract (BRIEF.md
// Part I §5) mirrored on the server side: nothing about serving this data
// should let a client influence detection behavior.
//
// The collector's own pipeline (Application, EventMonitor) is completely
// unaware this class exists beyond appending to the TelemetryStore it's
// handed; ApiServer only ever reads from it.
class ApiServer {
   public:
    ApiServer(TelemetryStore& store, ApiSettings settings, Logger& logger);
    ~ApiServer();

    ApiServer(const ApiServer&) = delete;
    ApiServer& operator=(const ApiServer&) = delete;

    // Starts the listener thread. Safe to call even when settings.enabled
    // is false -- it just does nothing, so callers don't need an if-guard
    // at every call site.
    void Start();

    // Requests the listener stop and joins the thread. Safe to call
    // multiple times or without a prior Start().
    void Stop();

   private:
    void RegisterRoutes();
    void Run();

    TelemetryStore& store_;
    ApiSettings settings_;
    Logger& logger_;

    std::unique_ptr<httplib::Server> server_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

}  // namespace sentinelforge
