#include "widgets/CollapsibleSection.h"

#include <QHBoxLayout>
#include <QLabel>

namespace sentinelforge {

CollapsibleSection::CollapsibleSection(const QString& title, bool expanded, QWidget* parent)
    : QFrame(parent), expanded_(expanded) {
    setObjectName(QStringLiteral("CollapsibleSection"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 8, 0, 8);
    root->setSpacing(8);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(6);

    toggle_ = new QToolButton(header);
    toggle_->setObjectName(QStringLiteral("SectionToggle"));
    toggle_->setAutoRaise(true);
    toggle_->setCursor(Qt::PointingHandCursor);
    connect(toggle_, &QToolButton::clicked, this, [this]() { setExpanded(!expanded_); });
    headerLayout->addWidget(toggle_);

    title_ = new QLabel(title.toUpper(), header);
    title_->setObjectName(QStringLiteral("MicroLabel"));
    headerLayout->addWidget(title_);
    headerLayout->addStretch();
    root->addWidget(header);

    bodyHost_ = new QWidget(this);
    bodyLayout_ = new QVBoxLayout(bodyHost_);
    bodyLayout_->setContentsMargins(0, 0, 0, 0);
    bodyLayout_->setSpacing(6);
    root->addWidget(bodyHost_);

    sync();
}

void CollapsibleSection::setContent(QWidget* body) {
    while (QLayoutItem* item = bodyLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    if (body) {
        bodyLayout_->addWidget(body);
    }
}

void CollapsibleSection::setExpanded(bool expanded) {
    expanded_ = expanded;
    sync();
}

void CollapsibleSection::sync() {
    toggle_->setText(expanded_ ? QStringLiteral("▾") : QStringLiteral("▸"));
    bodyHost_->setVisible(expanded_);
}

}  // namespace sentinelforge
