#include "widgets/StatusStrip.h"

#include <QDateTime>
#include <QEasingCurve>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariantAnimation>

#include "Theme.h"
#include "util/TimestampFormat.h"

namespace sentinelforge {

StatusChip::StatusChip(const QString& label, QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("StatusChip"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(3);

    auto* top = new QHBoxLayout();
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(6);

    dot_ = new QLabel(this);
    dot_->setFixedSize(8, 8);
    dot_->setAttribute(Qt::WA_StyledBackground, true);
    dot_->hide();
    top->addWidget(dot_, 0, Qt::AlignVCenter);

    label_ = new QLabel(label.toUpper(), this);
    label_->setObjectName(QStringLiteral("MicroLabel"));
    top->addWidget(label_);
    top->addStretch();
    layout->addLayout(top);

    value_ = new QLabel(QStringLiteral("—"), this);
    value_->setObjectName(QStringLiteral("ChipValue"));
    value_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    layout->addWidget(value_);

    anim_ = new QVariantAnimation(this);
    anim_->setDuration(300);
    anim_->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        value_->setText(QString::number(v.toDouble(), 'f', decimals_));
    });
}

void StatusChip::setValue(const QString& value) { value_->setText(value); }

void StatusChip::setNumericValue(double value, int decimals) {
    decimals_ = decimals;
    const double delta = qAbs(value - currentNumeric_);
    if (currentNumeric_ > 0.0 && delta > currentNumeric_ * 0.5) {
        anim_->stop();
        currentNumeric_ = value;
        value_->setText(QString::number(value, 'f', decimals));
        return;
    }
    anim_->stop();
    anim_->setStartValue(currentNumeric_);
    anim_->setEndValue(value);
    currentNumeric_ = value;
    anim_->start();
}

void StatusChip::setToolTipText(const QString& tip) { setToolTip(tip); }

void StatusChip::setStatusDotColor(const QString& colorCss) {
    dot_->show();
    dot_->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 4px;").arg(colorCss));
}

StatusStrip::StatusStrip(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("StatusStrip"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);
    setFixedHeight(52);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    collectorChip_ = new StatusChip(QStringLiteral("Collector"), this);
    threatChip_ = new StatusChip(QStringLiteral("Threat"), this);
    rulesChip_ = new StatusChip(QStringLiteral("Rules"), this);
    epsChip_ = new StatusChip(QStringLiteral("Events/sec"), this);
    eventsChip_ = new StatusChip(QStringLiteral("Events"), this);
    detectionsChip_ = new StatusChip(QStringLiteral("Detections"), this);
    alertsChip_ = new StatusChip(QStringLiteral("Alerts"), this);

    for (StatusChip* chip : {collectorChip_, threatChip_, rulesChip_, epsChip_, eventsChip_,
                             detectionsChip_, alertsChip_}) {
        layout->addWidget(chip);
    }
    layout->addStretch();

    collectorChip_->setValue(QStringLiteral("Disconnected"));
    collectorChip_->setStatusDotColor(QLatin1String(theme::StatusIdle));
    threatChip_->setValue(QStringLiteral("Low"));
}

void StatusStrip::setLiveViewState(LiveViewState state, const QString& detail) {
    liveState_ = state;
    liveDetail_ = detail;
    applyLiveState();
}

void StatusStrip::setThreatLevel(Severity level, const QString& tip) {
    threatChip_->setValue(severityLabel(level));
    threatChip_->setToolTipText(tip);
}

void StatusStrip::applyLiveState() {
    QString label = QStringLiteral("Disconnected");
    QString color = QLatin1String(theme::StatusIdle);
    switch (liveState_) {
        case LiveViewState::Connected:
            label = QStringLiteral("Connected");
            color = QLatin1String(theme::StatusOk);
            break;
        case LiveViewState::Receiving:
            label = QStringLiteral("Receiving");
            color = QLatin1String(theme::StatusOk);
            break;
        case LiveViewState::Paused:
            label = QStringLiteral("Paused");
            color = QLatin1String(theme::StatusWarn);
            break;
        case LiveViewState::Buffering:
            label = QStringLiteral("Buffering");
            color = QLatin1String(theme::StatusWarn);
            break;
        case LiveViewState::Connecting:
            label = QStringLiteral("Connecting");
            color = QLatin1String(theme::StatusWarn);
            break;
        case LiveViewState::Reconnecting:
            label = QStringLiteral("Reconnecting");
            color = QLatin1String(theme::StatusWarn);
            break;
        case LiveViewState::Failed:
            label = QStringLiteral("Failed");
            color = QLatin1String(theme::StatusError);
            break;
        default:
            break;
    }
    collectorChip_->setValue(label);
    collectorChip_->setStatusDotColor(color);

    QString tip = liveDetail_;
    if (lastUpdateMs_ > 0) {
        tip += QStringLiteral("\nLast update: %1")
                   .arg(timefmt::tooltipTime(lastUpdateMs_, QDateTime::currentMSecsSinceEpoch()));
    }
    if (connectedSinceMs_ > 0 && connectionState_ == ConnectionState::Connected) {
        const qint64 upMs = QDateTime::currentMSecsSinceEpoch() - connectedSinceMs_;
        tip += QStringLiteral("\nUptime: %1 min").arg(upMs / 60'000);
    }
    collectorChip_->setToolTipText(tip.trimmed());
}

void StatusStrip::onStatsUpdated(CollectorStats stats) {
    lastUpdateMs_ = QDateTime::currentMSecsSinceEpoch();
    rulesChip_->setNumericValue(stats.rulesLoaded + stats.sigmaRulesLoaded, 0);
    rulesChip_->setToolTipText(
        QStringLiteral("Native: %1\nSigma: %2\nCorrelation: %3")
            .arg(stats.rulesLoaded)
            .arg(stats.sigmaRulesLoaded)
            .arg(stats.correlationEnabled ? QStringLiteral("enabled")
                                          : QStringLiteral("disabled")));
    epsChip_->setNumericValue(stats.eventsPerSecond, 1);
    eventsChip_->setNumericValue(static_cast<double>(stats.eventsProcessed), 0);
    detectionsChip_->setNumericValue(static_cast<double>(stats.detections), 0);
    alertsChip_->setNumericValue(static_cast<double>(stats.correlationAlerts), 0);

    if (liveState_ != LiveViewState::Paused && liveState_ != LiveViewState::Buffering &&
        connectionState_ == ConnectionState::Connected) {
        liveState_ = LiveViewState::Receiving;
    }
    applyLiveState();
}

void StatusStrip::onConnectionStateChanged(ConnectionState state, const QString& detail) {
    connectionState_ = state;
    liveDetail_ = detail;
    switch (state) {
        case ConnectionState::Connected:
            if (connectedSinceMs_ == 0) {
                connectedSinceMs_ = QDateTime::currentMSecsSinceEpoch();
            }
            if (liveState_ != LiveViewState::Paused && liveState_ != LiveViewState::Buffering) {
                liveState_ = LiveViewState::Connected;
            }
            break;
        case ConnectionState::Connecting:
            liveState_ = LiveViewState::Connecting;
            connectedSinceMs_ = 0;
            break;
        case ConnectionState::Reconnecting:
            liveState_ = LiveViewState::Reconnecting;
            break;
        case ConnectionState::Failed:
            liveState_ = LiveViewState::Failed;
            connectedSinceMs_ = 0;
            break;
        default:
            liveState_ = LiveViewState::Disconnected;
            connectedSinceMs_ = 0;
            break;
    }
    applyLiveState();
}

}  // namespace sentinelforge
