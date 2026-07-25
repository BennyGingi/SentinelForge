#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>

#include "telemetry/ITelemetrySource.h"

namespace sentinelforge {

// Real ITelemetrySource implementation (Issue #035 phase 2): polls the
// collector's read-only HTTP API (docs/architecture/collector-api.md)
// instead of generating mock data. Same threading contract as
// MockTelemetrySource -- constructed on the UI thread, moved to a worker
// QThread by MainWindow, start()/stop() invoked as queued slots from there.
// QNetworkAccessManager is created in the constructor as a child QObject
// (like MockTelemetrySource's QTimer members), so moveToThread carries it
// to the worker thread correctly; nothing here ever touches the network
// from the UI thread.
class CollectorTelemetrySource final : public ITelemetrySource {
    Q_OBJECT
   public:
    explicit CollectorTelemetrySource(QUrl baseUrl = QUrl(QStringLiteral("http://127.0.0.1:8787")),
                                      QObject* parent = nullptr);

   public slots:
    void start() override;
    void stop() override;

   public:
    // No /rules endpoint exists on the collector API yet (phase 1 shipped
    // only /health, /stats, /detections, /correlations, /logs) -- returns a
    // default-constructed RuleInfo with just ruleId echoed back. Documented
    // gap, not fabricated data; see docs/architecture/collector-api.md.
    Q_INVOKABLE RuleInfo ruleInfo(const QString& ruleId) const override;

   private slots:
    void poll();

   private:
    // isTickRequest distinguishes the 4 requests a timer tick fires
    // (participate in the once-per-tick connection-state verdict) from
    // "more": true continuation follow-ups draining a backlog page (don't --
    // see onRequestOutcome).
    void pollDetections(bool isTickRequest = true);
    void pollCorrelations(bool isTickRequest = true);
    void pollLogs(bool isTickRequest = true);
    void pollStats();

    void onRequestOutcome(bool succeeded, bool isTickRequest);
    void onTickSucceeded();
    void onTickFailed();
    void applyBackoffInterval();

    QUrl baseUrl_;
    QNetworkAccessManager* networkManager_;
    QTimer pollTimer_;
    bool running_ = false;

    quint64 detectionCursor_ = 0;
    quint64 correlationCursor_ = 0;
    quint64 logCursor_ = 0;

    // Aggregates the 4 tick-request outcomes into a single verdict per
    // timer tick -- see onRequestOutcome. Without this, a single tick where
    // the collector is simply gone fires 4 near-simultaneous failures and
    // blows straight past consecutiveFailedTicks_ >= kFailedThreshold in
    // one tick, and "Reconnecting" becomes unobservable in practice.
    int tickRepliesPending_ = 0;
    bool tickHadFailure_ = false;

    int consecutiveFailedTicks_ = 0;
    ConnectionState currentState_ = ConnectionState::Disconnected;

    static constexpr int kPollIntervalMs = 500;
    static constexpr int kMaxBackoffMs = 8000;
    static constexpr int kFailedThreshold = 3;
};

}  // namespace sentinelforge
