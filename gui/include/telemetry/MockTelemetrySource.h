#pragma once

#include <QMutex>
#include <QHash>
#include <QTimer>
#include <QVector>

#include "telemetry/ITelemetrySource.h"

namespace sentinelforge {

class MockTelemetrySource final : public ITelemetrySource {
    Q_OBJECT
public:
    explicit MockTelemetrySource(QObject* parent = nullptr);

public slots:
    void start() override;
    void stop() override;
    void injectBurst(int count);

public:
    Q_INVOKABLE RuleInfo ruleInfo(const QString& ruleId) const override;

private slots:
    void onGenerateTick();
    void onFlushTick();
    void onStatsTick();

private:
    void buildRuleCatalog();
    void emitSeedBatch();
    Detection makeDetection();
    Detection makeSeedDetection(int index, const QDateTime& when);
    CorrelationAlert makeAlert();
    CorrelationAlert makeSeedAlert(int index, const QDateTime& when);
    LogLine makeLogLine(LogLevel level, const QString& message);
    QString makeRawJson(const Detection& d) const;

    QTimer generateTimer_;
    QTimer flushTimer_;
    QTimer statsTimer_;
    QMutex mutex_;
    QVector<Detection> pendingDetections_;
    QVector<CorrelationAlert> pendingAlerts_;
    QVector<LogLine> pendingLogs_;
    QHash<QString, RuleInfo> rules_;
    QVector<QString> recentDetectionIds_;
    CollectorStats stats_;
    bool running_ = false;
    bool seeded_ = false;
    quint64 sequence_ = 0;
};

}  // namespace sentinelforge
