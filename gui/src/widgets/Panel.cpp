#include "widgets/Panel.h"

namespace sentinelforge {

Panel::Panel(const QString& title, QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("Panel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);

    rootLayout_ = new QVBoxLayout(this);
    rootLayout_->setContentsMargins(16, 16, 16, 16);
    rootLayout_->setSpacing(8);

    headerLayout_ = new QHBoxLayout();
    headerLayout_->setContentsMargins(0, 0, 0, 0);
    headerLayout_->setSpacing(8);

    titleLabel_ = new QLabel(title, this);
    titleLabel_->setObjectName(QStringLiteral("PanelTitle"));
    headerLayout_->addWidget(titleLabel_);
    headerLayout_->addStretch();
    rootLayout_->addLayout(headerLayout_);

    contentLayout_ = new QVBoxLayout();
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    contentLayout_->setSpacing(0);
    rootLayout_->addLayout(contentLayout_, 1);
}

void Panel::setContent(QWidget* body) {
    while (QLayoutItem* item = contentLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    if (body) {
        contentLayout_->addWidget(body, 1);
    }
}

void Panel::setHeaderAction(QWidget* action) {
    if (action) {
        headerLayout_->addWidget(action);
    }
}

}  // namespace sentinelforge
