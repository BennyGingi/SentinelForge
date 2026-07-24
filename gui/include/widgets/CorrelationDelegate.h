#pragma once

#include <QColor>
#include <QFont>
#include <QStyledItemDelegate>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

class CorrelationDelegate final : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CorrelationDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    void colorsFor(Severity severity, QColor* fg, QColor* bg) const;
    QFont pillFont_;
    QFont monoFont_;
    QFont uiFont_;
};

}  // namespace sentinelforge
