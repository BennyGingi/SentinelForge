#include "widgets/SeverityRailDelegate.h"

#include <QPainter>

#include "Theme.h"

namespace sentinelforge {

SeverityRailDelegate::SeverityRailDelegate(QObject* parent) : QStyledItemDelegate(parent) {
    pillFont_ = QFont(QStringLiteral("Inter"), 11, QFont::DemiBold);
    pillFont_.setFamilies({QStringLiteral("Inter"), QStringLiteral("Segoe UI"),
                           QStringLiteral("sans-serif")});
    monoFont_ = QFont(QStringLiteral("Cascadia Mono"), 12);
    monoFont_.setFamilies({QStringLiteral("JetBrains Mono"), QStringLiteral("Cascadia Mono"),
                           QStringLiteral("Consolas"), QStringLiteral("monospace")});
    uiFont_ = QFont(QStringLiteral("Inter"), 12);
    uiFont_.setFamilies(
        {QStringLiteral("Inter"), QStringLiteral("Segoe UI"), QStringLiteral("sans-serif")});
}

void SeverityRailDelegate::colorsFor(Severity severity, QColor* fg, QColor* bg) const {
    switch (severity) {
        case Severity::Critical:
            *fg = QColor(QLatin1String(theme::SevCriticalFg));
            *bg = QColor(QLatin1String(theme::SevCriticalBg));
            break;
        case Severity::High:
            *fg = QColor(QLatin1String(theme::SevHighFg));
            *bg = QColor(QLatin1String(theme::SevHighBg));
            break;
        case Severity::Medium:
            *fg = QColor(QLatin1String(theme::SevMediumFg));
            *bg = QColor(QLatin1String(theme::SevMediumBg));
            break;
        case Severity::Low:
            *fg = QColor(QLatin1String(theme::SevLowFg));
            *bg = QColor(QLatin1String(theme::SevLowBg));
            break;
        case Severity::Info:
        default:
            *fg = QColor(QLatin1String(theme::SevInfoFg));
            *bg = QColor(QLatin1String(theme::SevInfoBg));
            break;
    }
}

void SeverityRailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
    painter->save();

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const auto severity =
        index.data(Qt::UserRole + 1).value<Severity>();  // SeverityRole
    QColor sevFg;
    QColor sevBg;
    colorsFor(severity, &sevFg, &sevBg);

    // Background
    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, QColor(QLatin1String(theme::BgRaised)));
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor(QLatin1String(theme::BgRaised)));
    } else {
        painter->fillRect(opt.rect, QColor(QLatin1String(theme::BgSurface)));
    }

    // Severity rail (full row height, leftmost column only paints the rail
    // for the whole row — we paint rail on every cell for simplicity so the
    // left edge is always covered when column 0 is visible).
    if (index.column() == 0) {
        QRect rail(opt.rect.left(), opt.rect.top(), railWidth_, opt.rect.height());
        painter->fillRect(rail, sevFg);
        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(QRect(rail.left() + 1, rail.top() + 2, 2, rail.height() - 4),
                              QColor(QLatin1String(theme::Accent)));
        }
    }

    QRect content = opt.rect.adjusted(index.column() == 0 ? railWidth_ + 8 : 12, 0, -12, 0);

    if (index.column() == 1) {
        // Severity pill
        const QString label = index.data(Qt::DisplayRole).toString();
        painter->setFont(pillFont_);
        const QFontMetrics fm(pillFont_);
        const int textW = fm.horizontalAdvance(label);
        const int pillH = 18;
        const int pillW = textW + 16;
        QRect pill(content.left(), content.center().y() - pillH / 2, pillW, pillH);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(sevBg);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(pill, 10, 10);
        painter->setPen(sevFg);
        painter->drawText(pill, Qt::AlignCenter, label);
    } else {
        const bool mono = index.column() == 0 || index.column() == 3 || index.column() == 4;
        painter->setFont(mono ? monoFont_ : uiFont_);
        painter->setPen(index.column() == 4 ? QColor(QLatin1String(theme::TextSecondary))
                                            : QColor(QLatin1String(theme::TextPrimary)));
        painter->drawText(content, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());
    }

    painter->restore();
}

}  // namespace sentinelforge
