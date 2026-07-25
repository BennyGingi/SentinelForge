#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace sentinelforge {

// API-facing detection record (Issue #035). Deliberately not the GUI's
// Detection type -- this is a collector-owned shape, built from
// NormalizedEvent + DetectionResult at the point a rule fires. pid/parentPid
// are not populated: NormalizedEvent does not carry them (EventNormalizer
// never copies Event::Pid() across), and extending that is a bigger change
// than this endpoint. Left at 0 rather than silently fabricated.
struct StoredDetection {
    std::uint64_t sequence = 0;
    std::string id;
    std::int64_t timestampMs = 0;
    std::string severity;
    std::string ruleName;
    std::string mitre;
    std::string processName;
    std::string parentProcess;
    std::string commandLine;
    std::string host;
    std::string user;
    std::string reason;
    std::uint32_t pid = 0;
    std::uint32_t parentPid = 0;
};

struct StoredCorrelationAlert {
    std::uint64_t sequence = 0;
    std::string id;
    std::int64_t timestampMs = 0;
    std::string title;
    std::string description;
    std::string severity;
    int confidence = 0;
    std::size_t matchedEventCount = 0;
    std::vector<std::string> mitreTechniques;
};

struct StoredLogLine {
    std::uint64_t sequence = 0;
    std::int64_t timestampMs = 0;
    std::string level;
    std::string component;
    std::string message;
};

// One page of a cursor query. cursor is the sequence of the last item
// included (or the current head if nothing new matched `since`) -- always
// the value a caller should pass as the next `since`, never the buffer's
// absolute head when `more` is true, so a client working through a large
// backlog is never skipped ahead past items it hasn't seen yet.
template <typename T>
struct CursorPage {
    std::uint64_t cursor = 0;
    std::vector<T> items;
    bool more = false;
};

struct TelemetryStats {
    std::uint64_t eventsProcessed = 0;
    std::uint64_t detections = 0;
    std::uint64_t correlationAlerts = 0;
    int rulesLoaded = 0;
    int sigmaRulesLoaded = 0;
    int correlationRulesLoaded = 0;
    double eventsPerSecond = 0.0;
    double pipelineLatencyMs = 0.0;
};

// Bounded, thread-safe, sequence-cursored telemetry buffer feeding the
// collector's HTTP API. Three independent ring buffers (detections, alerts,
// logs), each with its own monotonic sequence counter -- the cursor a
// GET /detections?since=N client passes only makes sense against the
// detections sequence, so keeping them separate (not one shared counter
// across all three streams) avoids a client's cursor silently meaning
// different things depending which endpoint issued it.
//
// Writers (the detection pipeline) must never block behind a slow HTTP
// reader: every method here only ever holds the mutex for a vector
// push_back/pop_front, never I/O, never a callback back out to a caller.
class TelemetryStore {
   public:
    static constexpr std::size_t kDetectionCap = 10000;
    static constexpr std::size_t kAlertCap = 5000;
    static constexpr std::size_t kLogCap = 10000;
    static constexpr std::size_t kMaxPageSize = 500;

    TelemetryStore();

    void AppendDetection(StoredDetection detection);
    void AppendAlert(StoredCorrelationAlert alert);
    void AppendLog(StoredLogLine line);

    CursorPage<StoredDetection> DetectionsSince(std::uint64_t since) const;
    CursorPage<StoredCorrelationAlert> AlertsSince(std::uint64_t since) const;
    CursorPage<StoredLogLine> LogsSince(std::uint64_t since) const;

    void SetRulesLoaded(int nativeAndSigma, int sigmaOnly, int correlationRules);
    void RecordEventProcessed(std::chrono::milliseconds latency);

    TelemetryStats Stats() const;
    std::chrono::steady_clock::time_point StartTime() const;

    // Converts an ISO-8601 UTC timestamp ("2026-07-20T14:32:07Z", the shape
    // NormalizedEvent::Timestamp() produces) to epoch milliseconds. Returns
    // 0 if the string doesn't parse -- callers already treat 0 as "unknown"
    // elsewhere in this API (e.g. an empty/malformed source timestamp).
    static std::int64_t ParseIso8601ToEpochMs(const std::string& timestamp);

   private:
    template <typename T>
    static void PushBounded(std::deque<T>& buffer, T item, std::size_t cap);

    template <typename T>
    static CursorPage<T> QuerySince(const std::deque<T>& buffer, std::uint64_t since,
                                    std::mutex& mutex);

    mutable std::mutex detectionsMutex_;
    std::deque<StoredDetection> detections_;
    std::uint64_t detectionSequence_ = 0;

    mutable std::mutex alertsMutex_;
    std::deque<StoredCorrelationAlert> alerts_;
    std::uint64_t alertSequence_ = 0;

    mutable std::mutex logsMutex_;
    std::deque<StoredLogLine> logs_;
    std::uint64_t logSequence_ = 0;

    mutable std::mutex statsMutex_;
    TelemetryStats stats_;
    std::deque<std::chrono::steady_clock::time_point> recentEventTimes_;

    const std::chrono::steady_clock::time_point startTime_;
};

}  // namespace sentinelforge
