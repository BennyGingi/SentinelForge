#include "pages/DashboardPage.h"

#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>
#include <span>

#include "ops/LiveOpsController.h"
#include "ops/ThreatScorer.h"
#include "widgets/CorrelationTableView.h"
#include "widgets/CriticalToast.h"
#include "widgets/DetectionTableView.h"
#include "widgets/LiveControlsBar.h"
#include "widgets/LogViewer.h"
#include "widgets/PerformancePanel.h"
#include "widgets/StatusStrip.h"

namespace sentinelforge {

DashboardPage::DashboardPage(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    statusStrip_ = new StatusStrip(this);
    root->addWidget(statusStrip_);

    liveControls_ = new LiveControlsBar(this);
    root->addWidget(liveControls_);

    liveOps_ = new LiveOpsController(this);
    threatScorer_ = new ThreatScorer(this);
    toast_ = new CriticalToast(this);

    connect(liveControls_, &LiveControlsBar::liveClicked, this, [this]() { setPaused(false); });
    connect(liveControls_, &LiveControlsBar::pauseClicked, this, [this]() { setPaused(true); });
    connect(liveControls_, &LiveControlsBar::resumeClicked, this, [this]() { setPaused(false); });
    connect(liveOps_, &LiveOpsController::pauseStateChanged, this,
            &DashboardPage::onPauseStateChanged);
    connect(liveOps_, &LiveOpsController::criticalDetected, toast_,
            &CriticalToast::showDetection);
    connect(threatScorer_, &ThreatScorer::levelChanged, this,
            [this](Severity level, const QString& detail) {
                statusStrip_->setThreatLevel(level, detail);
            });

    outerSplitter_ = new QSplitter(Qt::Vertical, this);
    outerSplitter_->setChildrenCollapsible(false);

    detections_ = new DetectionTableView(DetectionTableView::Mode::Live, outerSplitter_);
    outerSplitter_->addWidget(detections_);

    midSplitter_ = new QSplitter(Qt::Horizontal, outerSplitter_);
    midSplitter_->setChildrenCollapsible(false);
    correlation_ = new CorrelationTableView(midSplitter_);
    performance_ = new PerformancePanel(midSplitter_);
    midSplitter_->addWidget(correlation_);
    midSplitter_->addWidget(performance_);
    midSplitter_->setStretchFactor(0, 3);
    midSplitter_->setStretchFactor(1, 2);
    midSplitter_->setSizes({700, 400});
    outerSplitter_->addWidget(midSplitter_);

    logs_ = new LogViewer(outerSplitter_);
    outerSplitter_->addWidget(logs_);

    outerSplitter_->setStretchFactor(0, 42);
    outerSplitter_->setStretchFactor(1, 30);
    outerSplitter_->setStretchFactor(2, 28);
    outerSplitter_->setSizes({378, 270, 252});
    restoreSplitters();
    root->addWidget(outerSplitter_, 1);

    connect(outerSplitter_, &QSplitter::splitterMoved, this, [this](int, int) { persistSplitters(); });
    connect(midSplitter_, &QSplitter::splitterMoved, this, [this](int, int) { persistSplitters(); });

    connect(detections_, &DetectionTableView::detectionActivated, this,
            &DashboardPage::detectionActivated);
    connect(detections_, &DetectionTableView::openRuleRequested, this,
            &DashboardPage::openRuleRequested);
    connect(detections_, &DetectionTableView::openMitreRequested, this,
            &DashboardPage::openMitreRequested);
    connect(correlation_, &CorrelationTableView::alertActivated, this,
            &DashboardPage::alertActivated);

    connect(liveOps_, &LiveOpsController::detectionsReady, this,
            [this](const QVector<Detection>& batch) {
                detections_->model()->appendBatch(
                    std::span<const Detection>(batch.constData(), static_cast<size_t>(batch.size())));
                threatScorer_->noteDetections(batch);
            });
    connect(liveOps_, &LiveOpsController::alertsReady, this,
            [this](const QVector<CorrelationAlert>& batch) {
                correlation_->model()->appendBatch(std::span<const CorrelationAlert>(
                    batch.constData(), static_cast<size_t>(batch.size())));
            });
    connect(liveOps_, &LiveOpsController::logsReady, logs_, &LogViewer::appendLines);
}

DetectionModel* DashboardPage::detectionModel() const { return detections_->model(); }

CorrelationModel* DashboardPage::correlationModel() const { return correlation_->model(); }

void DashboardPage::setPaused(bool paused) { liveOps_->setPaused(paused); }

void DashboardPage::togglePaused() { liveOps_->togglePaused(); }

bool DashboardPage::isPaused() const { return liveOps_->isPaused(); }

void DashboardPage::onPauseStateChanged(bool paused, int buffered, int dropped) {
    liveControls_->setPaused(paused);
    liveControls_->setBufferStats(buffered, dropped);
    if (paused) {
        statusStrip_->setLiveViewState(LiveViewState::Paused,
                                       QStringLiteral("View paused — collector still receiving"));
    } else if (buffered > 0) {
        statusStrip_->setLiveViewState(LiveViewState::Buffering,
                                       QStringLiteral("Flushing view buffer"));
    } else {
        statusStrip_->setLiveViewState(LiveViewState::Receiving,
                                       QStringLiteral("Live view updating"));
    }
}

void DashboardPage::persistSplitters() {
    QSettings settings(QStringLiteral("SentinelForge"), QStringLiteral("Desktop"));
    settings.setValue(QStringLiteral("dashboardOuterSplitter"), outerSplitter_->saveState());
    settings.setValue(QStringLiteral("dashboardMidSplitter"), midSplitter_->saveState());
}

void DashboardPage::restoreSplitters() {
    QSettings settings(QStringLiteral("SentinelForge"), QStringLiteral("Desktop"));
    const QByteArray outer = settings.value(QStringLiteral("dashboardOuterSplitter")).toByteArray();
    const QByteArray mid = settings.value(QStringLiteral("dashboardMidSplitter")).toByteArray();
    if (!outer.isEmpty()) {
        outerSplitter_->restoreState(outer);
    }
    if (!mid.isEmpty()) {
        midSplitter_->restoreState(mid);
    }
}

void DashboardPage::onDetections(const QVector<Detection>& batch) {
    liveOps_->ingestDetections(batch);
}

void DashboardPage::onAlerts(const QVector<CorrelationAlert>& batch) {
    liveOps_->ingestAlerts(batch);
}

void DashboardPage::onLogs(const QVector<LogLine>& batch) { liveOps_->ingestLogs(batch); }

void DashboardPage::onStats(CollectorStats stats) {
    statusStrip_->onStatsUpdated(stats);
    performance_->onStatsUpdated(stats);
}

void DashboardPage::onConnectionState(ConnectionState state, const QString& detail) {
    statusStrip_->onConnectionStateChanged(state, detail);
}

}  // namespace sentinelforge
