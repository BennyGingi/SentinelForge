#include "widgets/LogViewer.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QVBoxLayout>

#include "Theme.h"
#include "util/TimestampFormat.h"
#include "widgets/Panel.h"

namespace sentinelforge {

LogViewer::LogViewer(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    panel_ = new Panel(QStringLiteral("Live logs"), this);

    auto* actions = new QWidget(panel_);
    auto* actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(8);

    searchField_ = new QLineEdit(actions);
    searchField_->setPlaceholderText(QStringLiteral("Filter logs…"));
    searchField_->setClearButtonEnabled(true);
    connect(searchField_, &QLineEdit::textChanged, this, [this](const QString&) { applyFilter(); });
    actionsLayout->addWidget(searchField_, 1);

    levelFilter_ = new QComboBox(actions);
    levelFilter_->addItem(QStringLiteral("All levels"), -1);
    levelFilter_->addItem(QStringLiteral("INFO+"), static_cast<int>(LogLevel::Info));
    levelFilter_->addItem(QStringLiteral("WARN+"), static_cast<int>(LogLevel::Warn));
    levelFilter_->addItem(QStringLiteral("ERROR+"), static_cast<int>(LogLevel::Error));
    connect(levelFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { applyFilter(); });
    actionsLayout->addWidget(levelFilter_);

    copyButton_ = new QPushButton(QStringLiteral("Copy"), actions);
    connect(copyButton_, &QPushButton::clicked, this, &LogViewer::copyVisible);
    actionsLayout->addWidget(copyButton_);

    clearButton_ = new QPushButton(QStringLiteral("Clear"), actions);
    connect(clearButton_, &QPushButton::clicked, this, [this]() {
        buffer_.clear();
        editor_->clear();
    });
    actionsLayout->addWidget(clearButton_);

    resumeButton_ = new QPushButton(QStringLiteral("Resume scroll"), actions);
    resumeButton_->setVisible(false);
    connect(resumeButton_, &QPushButton::clicked, this, [this]() {
        autoScroll_ = true;
        resumeButton_->setVisible(false);
        editor_->verticalScrollBar()->setValue(editor_->verticalScrollBar()->maximum());
    });
    actionsLayout->addWidget(resumeButton_);
    panel_->setHeaderAction(actions);

    editor_ = new QPlainTextEdit(panel_);
    editor_->setObjectName(QStringLiteral("LogViewer"));
    editor_->setAttribute(Qt::WA_StyledBackground, true);
    editor_->setReadOnly(true);
    editor_->setMaximumBlockCount(5000);
    editor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    panel_->setContent(editor_);
    root->addWidget(panel_, 1);

    connect(editor_->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &LogViewer::onManualScroll);
}

void LogViewer::focusEditor() { editor_->setFocus(Qt::ShortcutFocusReason); }

QString LogViewer::formatLineHtml(const LogLine& line) const {
    QString spine = QLatin1String(theme::SevInfoFg);
    switch (line.level) {
        case LogLevel::Warn:
            spine = QLatin1String(theme::SevMediumFg);
            break;
        case LogLevel::Error:
        case LogLevel::Fatal:
            spine = QLatin1String(theme::SevCriticalFg);
            break;
        case LogLevel::Debug:
        case LogLevel::Trace:
            spine = QLatin1String(theme::TextMuted);
            break;
        default:
            spine = QLatin1String(theme::SevLowFg);
            break;
    }

    return QStringLiteral(
               "<p style='margin:0; padding:1px 0 1px 8px; border-left:2px solid %1;'>"
               "<span style='color:%2'>%3</span> "
               "<span style='color:%1'>[%4]</span> "
               "<span style='color:%5'>[%6]</span> "
               "<span style='color:%7'>%8</span>"
               "</p>")
        .arg(spine, QLatin1String(theme::TextMuted), timefmt::logClock(line.timestampMs),
             logLevelLabel(line.level), QLatin1String(theme::TextSecondary), line.component,
             QLatin1String(theme::TextPrimary), line.message.toHtmlEscaped());
}

void LogViewer::appendLines(const QVector<LogLine>& lines) {
    for (const LogLine& line : lines) {
        buffer_.push_back(line);
    }
    while (buffer_.size() > 5000) {
        buffer_.removeFirst();
    }

    const int minLevel = levelFilter_->currentData().toInt();
    const QString needle = searchField_->text().trimmed().toLower();
    for (const LogLine& line : lines) {
        if (minLevel >= 0 && static_cast<int>(line.level) < minLevel) {
            continue;
        }
        if (!needle.isEmpty() && !line.message.toLower().contains(needle) &&
            !line.component.toLower().contains(needle)) {
            continue;
        }
        editor_->appendHtml(formatLineHtml(line));
    }

    if (autoScroll_) {
        editor_->verticalScrollBar()->setValue(editor_->verticalScrollBar()->maximum());
    }
}

void LogViewer::applyFilter() {
    editor_->clear();
    const int minLevel = levelFilter_->currentData().toInt();
    const QString needle = searchField_->text().trimmed().toLower();
    for (const LogLine& line : buffer_) {
        if (minLevel >= 0 && static_cast<int>(line.level) < minLevel) {
            continue;
        }
        if (!needle.isEmpty() && !line.message.toLower().contains(needle) &&
            !line.component.toLower().contains(needle)) {
            continue;
        }
        editor_->appendHtml(formatLineHtml(line));
    }
}

void LogViewer::copyVisible() { QApplication::clipboard()->setText(editor_->toPlainText()); }

void LogViewer::onManualScroll(int value) {
    const auto* bar = editor_->verticalScrollBar();
    if (value < bar->maximum()) {
        autoScroll_ = false;
        resumeButton_->setVisible(true);
    }
}

}  // namespace sentinelforge
