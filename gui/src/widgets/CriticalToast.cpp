#include "widgets/CriticalToast.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

namespace sentinelforge {

CriticalToast::CriticalToast(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("CriticalToast"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFixedWidth(280);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);

    title_ = new QLabel(QStringLiteral("CRITICAL"), this);
    title_->setObjectName(QStringLiteral("MicroLabel"));
    layout->addWidget(title_);

    body_ = new QLabel(this);
    body_->setWordWrap(true);
    layout->addWidget(body_);

    hideTimer_ = new QTimer(this);
    hideTimer_->setSingleShot(true);
    hideTimer_->setInterval(4000);
    connect(hideTimer_, &QTimer::timeout, this, &QWidget::hide);
    hide();
}

void CriticalToast::showDetection(const Detection& detection) {
    body_->setText(QStringLiteral("%1\n%2").arg(detection.ruleName, detection.host));
    if (QWidget* p = parentWidget()) {
        move(p->width() - width() - 24, 72);
    }
    show();
    raise();
    hideTimer_->start();
}

}  // namespace sentinelforge
