#pragma once

#include <QWidget>

#include "telemetry/TelemetryTypes.h"

class QLabel;
class QProgressBar;

namespace sentinelforge {

class SparklineWidget;
class Panel;

class PerformancePanel final : public QWidget {
    Q_OBJECT
public:
    explicit PerformancePanel(QWidget* parent = nullptr);

public slots:
    void onStatsUpdated(sentinelforge::CollectorStats stats);

private:
    Panel* panel_ = nullptr;
    QLabel* eventsValue_ = nullptr;
    QLabel* eventsUnit_ = nullptr;
    QLabel* latencyValue_ = nullptr;
    QLabel* latencyUnit_ = nullptr;
    QLabel* cpuValue_ = nullptr;
    QLabel* cpuUnit_ = nullptr;
    QLabel* memValue_ = nullptr;
    QLabel* memUnit_ = nullptr;
    QProgressBar* cpuBar_ = nullptr;
    QProgressBar* memBar_ = nullptr;
    SparklineWidget* eventsSpark_ = nullptr;
    SparklineWidget* latencySpark_ = nullptr;
};

}  // namespace sentinelforge
