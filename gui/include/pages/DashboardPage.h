#pragma once

#include <QWidget>

#include "models/CorrelationModel.h"
#include "models/DetectionModel.h"
#include "telemetry/TelemetryTypes.h"

class QSplitter;

namespace sentinelforge {

class StatusStrip;
class DetectionTableView;
class CorrelationTableView;
class PerformancePanel;
class LogViewer;
class LiveControlsBar;
class LiveOpsController;
class ThreatScorer;
class CriticalToast;

class DashboardPage final : public QWidget {
    Q_OBJECT
public:
    explicit DashboardPage(QWidget* parent = nullptr);

    DetectionModel* detectionModel() const;
    CorrelationModel* correlationModel() const;
    DetectionTableView* detectionTable() const { return detections_; }
    LogViewer* logViewer() const { return logs_; }
    LiveOpsController* liveOps() const { return liveOps_; }

    void setPaused(bool paused);
    void togglePaused();
    bool isPaused() const;

signals:
    void detectionActivated(sentinelforge::Detection detection);
    void alertActivated(sentinelforge::CorrelationAlert alert);
    void openRuleRequested(QString ruleId);
    void openMitreRequested(QString techniqueId);

public slots:
    void onDetections(const QVector<sentinelforge::Detection>& batch);
    void onAlerts(const QVector<sentinelforge::CorrelationAlert>& batch);
    void onLogs(const QVector<sentinelforge::LogLine>& batch);
    void onStats(sentinelforge::CollectorStats stats);
    void onConnectionState(sentinelforge::ConnectionState state, const QString& detail);

private:
    void persistSplitters();
    void restoreSplitters();
    void onPauseStateChanged(bool paused, int buffered, int dropped);

    StatusStrip* statusStrip_ = nullptr;
    LiveControlsBar* liveControls_ = nullptr;
    DetectionTableView* detections_ = nullptr;
    CorrelationTableView* correlation_ = nullptr;
    PerformancePanel* performance_ = nullptr;
    LogViewer* logs_ = nullptr;
    QSplitter* outerSplitter_ = nullptr;
    QSplitter* midSplitter_ = nullptr;
    LiveOpsController* liveOps_ = nullptr;
    ThreatScorer* threatScorer_ = nullptr;
    CriticalToast* toast_ = nullptr;
};

}  // namespace sentinelforge
