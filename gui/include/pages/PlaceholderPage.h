#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace sentinelforge {

class PlaceholderPage final : public QWidget {
public:
    // trailingWidget is optional (e.g. an "About" button on the Settings
    // placeholder); most placeholder pages pass nullptr.
    explicit PlaceholderPage(const QString& title, const QString& blurb,
                             QWidget* parent = nullptr, QWidget* trailingWidget = nullptr)
        : QWidget(parent) {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 24, 24, 24);
        auto* heading = new QLabel(title, this);
        heading->setObjectName(QStringLiteral("PanelTitle"));
        auto* body = new QLabel(blurb, this);
        body->setObjectName(QStringLiteral("EmptyState"));
        body->setWordWrap(true);
        layout->addWidget(heading);
        layout->addWidget(body);
        if (trailingWidget) {
            trailingWidget->setParent(this);
            layout->addSpacing(12);
            layout->addWidget(trailingWidget, 0, Qt::AlignLeft);
        }
        layout->addStretch();
    }
};

}  // namespace sentinelforge
