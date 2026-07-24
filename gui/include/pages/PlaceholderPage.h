#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace sentinelforge {

class PlaceholderPage final : public QWidget {
public:
    explicit PlaceholderPage(const QString& title, const QString& blurb,
                             QWidget* parent = nullptr)
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
        layout->addStretch();
    }
};

}  // namespace sentinelforge
