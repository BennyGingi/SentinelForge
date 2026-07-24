#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

// View-only pause buffer. Never stops ITelemetrySource.
// Caps and severity-aware eviction; drops are always counted and disclosed.
class LiveOpsController final : public QObject {
    Q_OBJECT
public:
    static constexpr int kDetectionCap = 5000;
    static constexpr int kAlertCap = 2000;
    static constexpr int kLogCap = 5000;
    static constexpr int kFlushBatch = 200;

    explicit LiveOpsController(QObject* parent = nullptr);

    bool isPaused() const { return paused_; }
    int bufferedDetections() const;
    int bufferedAlerts() const;
    int bufferedLogs() const;
    int droppedCount() const { return dropped_; }

public slots:
    void setPaused(bool paused);
    void togglePaused();
    void ingestDetections(const QVector<Detection>& batch);
    void ingestAlerts(const QVector<CorrelationAlert>& batch);
    void ingestLogs(const QVector<LogLine>& batch);

signals:
    void detectionsReady(QVector<sentinelforge::Detection> batch);
    void alertsReady(QVector<sentinelforge::CorrelationAlert> batch);
    void logsReady(QVector<sentinelforge::LogLine> batch);
    void pauseStateChanged(bool paused, int buffered, int dropped);
    void criticalDetected(sentinelforge::Detection detection);

private slots:
    void onFlushTick();

private:
    template <typename T>
    int appendWithEviction(QVector<T>& buffer, const QVector<T>& incoming, int cap);

    bool paused_ = false;
    int dropped_ = 0;
    QVector<Detection> detectionBuffer_;
    QVector<CorrelationAlert> alertBuffer_;
    QVector<LogLine> logBuffer_;
    QTimer flushTimer_;
};

}  // namespace sentinelforge
