#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

// Abstract telemetry boundary. Implementations live on a worker QThread.
// Almost all data arrives via queued batch signals.
//
// Exception: ruleInfo() is a synchronous accessor for static rule reference
// data (not live telemetry). Callers on the UI thread must use
// Qt::BlockingQueuedConnection when the source lives on a worker thread.
// Documented in docs/architecture/gui-architecture.md.
class ITelemetrySource : public QObject {
    Q_OBJECT
public:
    explicit ITelemetrySource(QObject* parent = nullptr) : QObject(parent) {}
    ~ITelemetrySource() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    // Static reference lookup — not a telemetry stream.
    // Call via BlockingQueuedConnection from the UI thread when the source
    // lives on a worker thread. See gui-architecture.md.
    virtual sentinelforge::RuleInfo ruleInfo(const QString& ruleId) const = 0;

signals:
    void detectionsReceived(QVector<sentinelforge::Detection> batch);
    void correlationAlertsReceived(QVector<sentinelforge::CorrelationAlert> batch);
    void logLinesReceived(QVector<sentinelforge::LogLine> batch);
    void statsUpdated(sentinelforge::CollectorStats stats);
    void connectionStateChanged(sentinelforge::ConnectionState state, QString detail);
};

}  // namespace sentinelforge
