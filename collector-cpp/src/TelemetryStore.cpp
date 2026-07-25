#include "TelemetryStore.h"

#include <algorithm>
#include <ctime>

#if defined(_WIN32)
#define SF_TIMEGM _mkgmtime
#else
#define SF_TIMEGM timegm
#endif

namespace sentinelforge {

namespace {
constexpr std::chrono::seconds kEventRateWindow{5};
}  // namespace

std::int64_t TelemetryStore::ParseIso8601ToEpochMs(const std::string& timestamp) {
    // Fixed-width "YYYY-MM-DDTHH:MM:SS[.mmm][Z]" -- NormalizedEvent's only
    // producer of this string (EventNormalizer) never varies the format, so
    // indexed substr/stoi is simpler and more portable than std::get_time
    // (MSVC's rejects the trailing 'Z' and fractional seconds) or sscanf
    // (deprecated by MSVC, /WX turns that into a hard build failure).
    if (timestamp.size() < 19 || timestamp[4] != '-' || timestamp[7] != '-' ||
        timestamp[10] != 'T' || timestamp[13] != ':' || timestamp[16] != ':') {
        return 0;
    }

    std::tm tm{};
    int milliseconds = 0;
    try {
        tm.tm_year = std::stoi(timestamp.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(timestamp.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(timestamp.substr(8, 2));
        tm.tm_hour = std::stoi(timestamp.substr(11, 2));
        tm.tm_min = std::stoi(timestamp.substr(14, 2));
        tm.tm_sec = std::stoi(timestamp.substr(17, 2));
        if (timestamp.size() >= 23 && timestamp[19] == '.') {
            milliseconds = std::stoi(timestamp.substr(20, 3));
        }
    } catch (const std::exception&) {
        return 0;
    }

    // timegm/_mkgmtime normalize out-of-range fields (day 40 rolls into the
    // next month, etc.) rather than rejecting them -- fine for mktime's
    // usual "compute a date" use, wrong for a parser whose job is to reject
    // garbage input from a raw telemetry string. Validate before it can
    // silently produce a plausible-looking but wrong epoch value.
    if (tm.tm_mon < 0 || tm.tm_mon > 11 || tm.tm_mday < 1 || tm.tm_mday > 31 || tm.tm_hour < 0 ||
        tm.tm_hour > 23 || tm.tm_min < 0 || tm.tm_min > 59 || tm.tm_sec < 0 || tm.tm_sec > 60) {
        return 0;
    }

    const std::time_t utcSeconds = SF_TIMEGM(&tm);
    if (utcSeconds == static_cast<std::time_t>(-1)) {
        return 0;
    }
    return static_cast<std::int64_t>(utcSeconds) * 1000 + milliseconds;
}

TelemetryStore::TelemetryStore() : startTime_(std::chrono::steady_clock::now()) {}

template <typename T>
void TelemetryStore::PushBounded(std::deque<T>& buffer, T item, std::size_t cap) {
    buffer.push_back(std::move(item));
    while (buffer.size() > cap) {
        buffer.pop_front();
    }
}

template <typename T>
CursorPage<T> TelemetryStore::QuerySince(const std::deque<T>& buffer, std::uint64_t since,
                                         std::mutex& mutex) {
    std::lock_guard<std::mutex> lock(mutex);

    CursorPage<T> page;
    if (buffer.empty()) {
        return page;
    }

    // buffer is ordered oldest-first with strictly increasing sequence, so
    // the first item with sequence > since starts the qualifying range.
    auto it = std::find_if(buffer.begin(), buffer.end(),
                           [since](const T& item) { return item.sequence > since; });

    const std::size_t available = static_cast<std::size_t>(std::distance(it, buffer.end()));
    const std::size_t take = std::min(available, TelemetryStore::kMaxPageSize);

    page.items.reserve(take);
    for (std::size_t i = 0; i < take; ++i, ++it) {
        page.items.push_back(*it);
    }

    page.more = available > take;
    // Cursor is the last item actually returned, so a paginating client's
    // next `since` continues exactly where this page left off. If nothing
    // qualified, the cursor is the buffer's current head -- the client is
    // already caught up, not stuck resending a stale `since`.
    page.cursor = page.items.empty() ? buffer.back().sequence : page.items.back().sequence;
    return page;
}

void TelemetryStore::AppendDetection(StoredDetection detection) {
    std::lock_guard<std::mutex> lock(detectionsMutex_);
    detection.sequence = ++detectionSequence_;
    if (detection.id.empty()) {
        // Collector has no natural stable detection id (Rule/DetectionResult
        // don't carry one -- see docs/architecture/regression-testing.md's
        // "What detection ID means here" for the same gap on the harness
        // side). The store's own sequence is unique and stable, so derive
        // one from it rather than push the problem onto every call site.
        detection.id = "det-" + std::to_string(detection.sequence);
    }
    PushBounded(detections_, std::move(detection), kDetectionCap);
}

void TelemetryStore::AppendAlert(StoredCorrelationAlert alert) {
    std::lock_guard<std::mutex> lock(alertsMutex_);
    alert.sequence = ++alertSequence_;
    if (alert.id.empty()) {
        alert.id = "alert-" + std::to_string(alert.sequence);
    }
    PushBounded(alerts_, std::move(alert), kAlertCap);
}

void TelemetryStore::AppendLog(StoredLogLine line) {
    std::lock_guard<std::mutex> lock(logsMutex_);
    line.sequence = ++logSequence_;
    PushBounded(logs_, std::move(line), kLogCap);
}

CursorPage<StoredDetection> TelemetryStore::DetectionsSince(std::uint64_t since) const {
    return QuerySince(detections_, since, detectionsMutex_);
}

CursorPage<StoredCorrelationAlert> TelemetryStore::AlertsSince(std::uint64_t since) const {
    return QuerySince(alerts_, since, alertsMutex_);
}

CursorPage<StoredLogLine> TelemetryStore::LogsSince(std::uint64_t since) const {
    return QuerySince(logs_, since, logsMutex_);
}

void TelemetryStore::SetRulesLoaded(int nativeAndSigma, int sigmaOnly, int correlationRules) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.rulesLoaded = nativeAndSigma;
    stats_.sigmaRulesLoaded = sigmaOnly;
    stats_.correlationRulesLoaded = correlationRules;
}

void TelemetryStore::RecordEventProcessed(std::chrono::milliseconds latency) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    ++stats_.eventsProcessed;
    stats_.pipelineLatencyMs = static_cast<double>(latency.count());

    const auto now = std::chrono::steady_clock::now();
    recentEventTimes_.push_back(now);
    while (!recentEventTimes_.empty() && now - recentEventTimes_.front() > kEventRateWindow) {
        recentEventTimes_.pop_front();
    }
    stats_.eventsPerSecond =
        static_cast<double>(recentEventTimes_.size()) / static_cast<double>(kEventRateWindow.count());
}

TelemetryStats TelemetryStore::Stats() const {
    std::lock_guard<std::mutex> statsLock(statsMutex_);
    TelemetryStats snapshot = stats_;

    // detections/correlationAlerts counts reflect what's actually been
    // appended, taken under each buffer's own mutex rather than duplicated
    // as a separately-maintained counter under statsMutex_ (which could
    // drift if a caller ever appended without going through Record*).
    {
        std::lock_guard<std::mutex> lock(detectionsMutex_);
        snapshot.detections = detectionSequence_;
    }
    {
        std::lock_guard<std::mutex> lock(alertsMutex_);
        snapshot.correlationAlerts = alertSequence_;
    }
    return snapshot;
}

std::chrono::steady_clock::time_point TelemetryStore::StartTime() const { return startTime_; }

}  // namespace sentinelforge
