#pragma once

#include <QColor>
#include <QFont>
#include <QPen>
#include <QStyledItemDelegate>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

// Paints the 3px severity rail and severity pill. Caches pens/fonts/colors.
class SeverityRailDelegate final : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit SeverityRailDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

private:
    void colorsFor(Severity severity, QColor* fg, QColor* bg) const;

    QFont pillFont_;
    QFont monoFont_;
    QFont uiFont_;
    int railWidth_ = 3;
};

}  // namespace sentinelforge
