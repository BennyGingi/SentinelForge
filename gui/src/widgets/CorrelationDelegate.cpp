#include "widgets/CorrelationDelegate.h"

#include <QPainter>

#include "Theme.h"

namespace sentinelforge {

CorrelationDelegate::CorrelationDelegate(QObject* parent) : QStyledItemDelegate(parent) {
    pillFont_ = QFont(QStringLiteral("Inter"), 11, QFont::DemiBold);
    monoFont_ = QFont(QStringLiteral("Cascadia Mono"), 12);
    monoFont_.setFamilies({QStringLiteral("JetBrains Mono"), QStringLiteral("Cascadia Mono"),
                           QStringLiteral("Consolas")});
    uiFont_ = QFont(QStringLiteral("Inter"), 12);
}

void CorrelationDelegate::colorsFor(Severity severity, QColor* fg, QColor* bg) const {
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
        default:
            *fg = QColor(QLatin1String(theme::SevInfoFg));
            *bg = QColor(QLatin1String(theme::SevInfoBg));
            break;
    }
}

void CorrelationDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    painter->save();
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const auto severity = index.data(Qt::UserRole + 1).value<Severity>();
    QColor sevFg;
    QColor sevBg;
    colorsFor(severity, &sevFg, &sevBg);

    if (opt.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
        painter->fillRect(opt.rect, QColor(QLatin1String(theme::BgRaised)));
    } else {
        painter->fillRect(opt.rect, QColor(QLatin1String(theme::BgSurface)));
    }

    if (index.column() == 0) {
        painter->fillRect(QRect(opt.rect.left(), opt.rect.top(), 3, opt.rect.height()), sevFg);
    }

    QRect content = opt.rect.adjusted(index.column() == 0 ? 11 : 12, 0, -12, 0);

    if (index.column() == 1) {
        const QString label = index.data(Qt::DisplayRole).toString();
        painter->setFont(pillFont_);
        const QFontMetrics fm(pillFont_);
        QRect pill(content.left(), content.center().y() - 9, fm.horizontalAdvance(label) + 16, 18);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(sevBg);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(pill, 10, 10);
        painter->setPen(sevFg);
        painter->drawText(pill, Qt::AlignCenter, label);
    } else if (index.column() == 3) {
        const int confidence = index.data(Qt::UserRole + 2).toInt();  // ConfidenceRole
        painter->setFont(monoFont_);
        painter->setPen(QColor(QLatin1String(theme::TextPrimary)));
        const QString pct = QStringLiteral("%1%").arg(confidence);
        const int textW = QFontMetrics(monoFont_).horizontalAdvance(pct) + 8;
        QRect barArea(content.left(), content.center().y() - 2, content.width() - textW, 4);
        painter->fillRect(barArea, QColor(QLatin1String(theme::BgRaised)));
        QRect fill(barArea.left(), barArea.top(),
                   qBound(0, barArea.width(), barArea.width() * confidence / 100), 4);
        painter->setBrush(QColor(QLatin1String(theme::Accent)));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(fill, 2, 2);
        painter->setPen(QColor(QLatin1String(theme::TextPrimary)));
        painter->drawText(QRect(barArea.right() + 4, content.top(), textW, content.height()),
                          Qt::AlignVCenter | Qt::AlignLeft, pct);
    } else {
        const bool mono = index.column() == 0 || index.column() == 4;
        painter->setFont(mono ? monoFont_ : uiFont_);
        painter->setPen(QColor(QLatin1String(theme::TextPrimary)));
        painter->drawText(content, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());
    }

    painter->restore();
}

}  // namespace sentinelforge
