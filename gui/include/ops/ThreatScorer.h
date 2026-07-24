#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

// Presentation heuristic — collector should own scoring later.
class ThreatScorer final : public QObject {
    Q_OBJECT
public:
    explicit ThreatScorer(QObject* parent = nullptr);

    void noteDetections(const QVector<Detection>& batch);
    Severity displayedLevel() const { return displayed_; }

signals:
    void levelChanged(sentinelforge::Severity level, QString detail);

private slots:
    void onTick();

private:
    Severity computeRaw() const;

    struct Sample {
        qint64 timestampMs = 0;
        Severity severity = Severity::Info;
    };

    QVector<Sample> samples_;
    Severity raw_ = Severity::Low;
    Severity displayed_ = Severity::Low;
    qint64 promoteSinceMs_ = 0;
    qint64 demoteSinceMs_ = 0;
    QTimer timer_;
};

}  // namespace sentinelforge
