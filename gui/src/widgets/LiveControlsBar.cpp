#include "widgets/LiveControlsBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

namespace sentinelforge {

LiveControlsBar::LiveControlsBar(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("LiveControlsBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    liveButton_ = new QPushButton(QStringLiteral("LIVE"), this);
    pauseButton_ = new QPushButton(QStringLiteral("PAUSE"), this);
    resumeButton_ = new QPushButton(QStringLiteral("RESUME"), this);
    liveButton_->setObjectName(QStringLiteral("LiveButtonActive"));
    pauseButton_->setObjectName(QStringLiteral("PauseButton"));
    resumeButton_->setObjectName(QStringLiteral("ResumeButton"));
    liveButton_->setCursor(Qt::PointingHandCursor);
    pauseButton_->setCursor(Qt::PointingHandCursor);
    resumeButton_->setCursor(Qt::PointingHandCursor);

    connect(liveButton_, &QPushButton::clicked, this, &LiveControlsBar::liveClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &LiveControlsBar::pauseClicked);
    connect(resumeButton_, &QPushButton::clicked, this, &LiveControlsBar::resumeClicked);

    bufferLabel_ = new QLabel(this);
    bufferLabel_->setObjectName(QStringLiteral("EmptyState"));

    layout->addWidget(liveButton_);
    layout->addWidget(pauseButton_);
    layout->addWidget(resumeButton_);
    layout->addWidget(bufferLabel_, 1);

    syncButtons();
}

void LiveControlsBar::setPaused(bool paused) {
    paused_ = paused;
    syncButtons();
}

void LiveControlsBar::setBufferStats(int buffered, int dropped) {
    if (!paused_ && buffered == 0) {
        bufferLabel_->clear();
        return;
    }
    if (paused_) {
        QString text = QStringLiteral("Paused · +%1 waiting").arg(buffered);
        if (dropped > 0) {
            text += QStringLiteral(" · %1 dropped").arg(dropped);
        }
        bufferLabel_->setText(text);
        bufferLabel_->setToolTip(QStringLiteral(
            "Collector keeps running. View buffer is bounded; drops are disclosed."));
    } else if (buffered > 0) {
        bufferLabel_->setText(QStringLiteral("Catching up · %1 remaining").arg(buffered));
    } else {
        bufferLabel_->clear();
    }
}

void LiveControlsBar::syncButtons() {
    liveButton_->setEnabled(paused_);
    pauseButton_->setEnabled(!paused_);
    resumeButton_->setEnabled(paused_);
    liveButton_->setObjectName(paused_ ? QStringLiteral("LiveButton")
                                        : QStringLiteral("LiveButtonActive"));
    pauseButton_->setObjectName(paused_ ? QStringLiteral("PauseButtonActive")
                                         : QStringLiteral("PauseButton"));
    liveButton_->style()->unpolish(liveButton_);
    liveButton_->style()->polish(liveButton_);
    pauseButton_->style()->unpolish(pauseButton_);
    pauseButton_->style()->polish(pauseButton_);
}

}  // namespace sentinelforge
