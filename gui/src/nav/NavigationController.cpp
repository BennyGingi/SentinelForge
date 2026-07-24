#include "nav/NavigationController.h"

#include <QFrame>
#include <QPainter>
#include <QSettings>
#include <QSvgRenderer>

#include "Theme.h"

namespace sentinelforge {

NavigationController::NavigationController(QFrame* rail, QStackedWidget* stack, QObject* parent)
    : QObject(parent), rail_(rail), stack_(stack) {
    railLayout_ = qobject_cast<QVBoxLayout*>(rail_->layout());
    if (!railLayout_) {
        railLayout_ = new QVBoxLayout(rail_);
        railLayout_->setContentsMargins(0, 8, 0, 8);
        railLayout_->setSpacing(4);
    }

    toggleButton_ = new QToolButton(rail_);
    toggleButton_->setObjectName(QStringLiteral("NavItem"));
    toggleButton_->setCheckable(false);
    toggleButton_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toggleButton_->setFixedHeight(40);
    toggleButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    toggleButton_->setCursor(Qt::PointingHandCursor);
    toggleButton_->setToolTip(QStringLiteral("Expand navigation"));
    connect(toggleButton_, &QToolButton::clicked, this, [this]() { setCollapsed(!collapsed_); });
    railLayout_->addWidget(toggleButton_);

    QSettings settings(QStringLiteral("SentinelForge"), QStringLiteral("Desktop"));
    collapsed_ = settings.value(QStringLiteral("navCollapsed"), true).toBool();
}

QIcon NavigationController::tintedIcon(const QString& path, const QColor& color) const {
    QSvgRenderer renderer(path);
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();
    return QIcon(pixmap);
}

int NavigationController::addPage(const QString& iconPath, const QString& title,
                                  PageFactory factory) {
    const int index = static_cast<int>(pages_.size());

    auto* button = new QToolButton(rail_);
    button->setObjectName(QStringLiteral("NavItem"));
    button->setCheckable(true);
    button->setAutoExclusive(true);
    button->setFixedHeight(40);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(title);

    // Insert before trailing stretch if present; otherwise append before toggle is wrong.
    // Pages go after toggle. Stretch is added by MainWindow between pages and settings.
    railLayout_->addWidget(button);

    PageEntry entry;
    entry.title = title;
    entry.iconPath = iconPath;
    entry.factory = std::move(factory);
    entry.button = button;
    pages_.push_back(std::move(entry));

    stack_->addWidget(new QWidget(stack_));
    connect(button, &QToolButton::clicked, this, [this, index]() { setCurrentIndex(index); });

    refreshButtonPresentation();

    if (index == 0) {
        button->setChecked(true);
        setCurrentIndex(0);
    }
    return index;
}

void NavigationController::setCollapsed(bool collapsed) {
    collapsed_ = collapsed;
    QSettings settings(QStringLiteral("SentinelForge"), QStringLiteral("Desktop"));
    settings.setValue(QStringLiteral("navCollapsed"), collapsed_);
    refreshButtonPresentation();
    emit collapsedChanged(collapsed_);
}

void NavigationController::refreshButtonPresentation() {
    const QColor muted{QString::fromLatin1(theme::TextMuted)};
    const QColor accent{QString::fromLatin1(theme::Accent)};
    const QColor secondary{QString::fromLatin1(theme::TextSecondary)};

    toggleButton_->setIcon(tintedIcon(QStringLiteral(":/icons/expand.svg"), secondary));
    toggleButton_->setToolButtonStyle(collapsed_ ? Qt::ToolButtonIconOnly
                                                 : Qt::ToolButtonTextBesideIcon);
    toggleButton_->setText(collapsed_ ? QString() : QStringLiteral("  Collapse"));
    toggleButton_->setToolTip(collapsed_ ? QStringLiteral("Expand navigation")
                                         : QStringLiteral("Collapse navigation"));

    for (PageEntry& entry : pages_) {
        const bool active = entry.button->isChecked();
        entry.button->setIcon(tintedIcon(entry.iconPath, active ? accent : muted));
        if (collapsed_) {
            entry.button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            entry.button->setText(QString());
        } else {
            entry.button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            // Escape mnemonics so ATT&CK does not eat the ampersand.
            QString label = entry.title;
            label.replace(QLatin1Char('&'), QStringLiteral("&&"));
            entry.button->setText(QStringLiteral("  %1").arg(label));
        }
    }
}

void NavigationController::ensurePageBuilt(int index) {
    if (index < 0 || index >= static_cast<int>(pages_.size())) {
        return;
    }
    PageEntry& entry = pages_[static_cast<size_t>(index)];
    if (entry.widget) {
        return;
    }
    entry.widget = entry.factory ? entry.factory() : new QWidget(stack_);
    QWidget* old = stack_->widget(index);
    stack_->removeWidget(old);
    old->deleteLater();
    stack_->insertWidget(index, entry.widget);
}

void NavigationController::setCurrentIndex(int index) {
    ensurePageBuilt(index);
    stack_->setCurrentIndex(index);
    if (index >= 0 && index < static_cast<int>(pages_.size())) {
        pages_[static_cast<size_t>(index)].button->setChecked(true);
    }
    refreshButtonPresentation();
    emit pageChanged(index);
}

int NavigationController::currentIndex() const { return stack_->currentIndex(); }

}  // namespace sentinelforge
