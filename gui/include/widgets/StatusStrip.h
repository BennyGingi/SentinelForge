#pragma once

#include <QFrame>
#include <QLabel>

#include "telemetry/TelemetryTypes.h"

class QVariantAnimation;

namespace sentinelforge {

enum class LiveViewState : quint8 {
    Disconnected = 0,
    Connecting,
    Connected,
    Receiving,
    Paused,
    Buffering,
    Reconnecting,
    Failed
};

class StatusChip final : public QFrame {
    Q_OBJECT
public:
    explicit StatusChip(const QString& label, QWidget* parent = nullptr);
    void setValue(const QString& value);
    void setNumericValue(double value, int decimals = 0);
    void setToolTipText(const QString& tip);
    void setStatusDotColor(const QString& colorCss);

private:
    QLabel* label_ = nullptr;
    QLabel* value_ = nullptr;
    QLabel* dot_ = nullptr;
    QVariantAnimation* anim_ = nullptr;
    double currentNumeric_ = 0.0;
    int decimals_ = 0;
};

class StatusStrip final : public QFrame {
    Q_OBJECT
public:
    explicit StatusStrip(QWidget* parent = nullptr);

    void setLiveViewState(LiveViewState state, const QString& detail = {});
    void setThreatLevel(Severity level, const QString& tip);

public slots:
    void onStatsUpdated(sentinelforge::CollectorStats stats);
    void onConnectionStateChanged(sentinelforge::ConnectionState state, const QString& detail);

private:
    void applyLiveState();

    StatusChip* collectorChip_ = nullptr;
    StatusChip* threatChip_ = nullptr;
    StatusChip* rulesChip_ = nullptr;
    StatusChip* epsChip_ = nullptr;
    StatusChip* eventsChip_ = nullptr;
    StatusChip* detectionsChip_ = nullptr;
    StatusChip* alertsChip_ = nullptr;

    LiveViewState liveState_ = LiveViewState::Disconnected;
    ConnectionState connectionState_ = ConnectionState::Disconnected;
    QString liveDetail_;
    qint64 lastUpdateMs_ = 0;
    qint64 connectedSinceMs_ = 0;
};

}  // namespace sentinelforge
