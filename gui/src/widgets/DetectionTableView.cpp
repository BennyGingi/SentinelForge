#include "widgets/DetectionTableView.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QEvent>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include "models/DetectionFilterProxy.h"
#include "widgets/Panel.h"
#include "widgets/SeverityRailDelegate.h"

namespace sentinelforge {

namespace {

QString detectionToTsv(const Detection& d) {
    return QStringLiteral("%1\t%2\t%3\t%4\t%5\t%6\t%7")
        .arg(QString::number(d.timestampMs), severityLabel(d.severity), d.ruleName, d.processName,
             d.techniqueId, d.host, d.user);
}

QString detectionToJson(const Detection& d) {
    if (!d.rawEventJson.isEmpty()) {
        return d.rawEventJson;
    }
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), d.id);
    obj.insert(QStringLiteral("ruleName"), d.ruleName);
    obj.insert(QStringLiteral("processName"), d.processName);
    obj.insert(QStringLiteral("host"), d.host);
    obj.insert(QStringLiteral("commandLine"), d.commandLine);
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

}  // namespace

DetectionTableView::DetectionTableView(Mode mode, QWidget* parent)
    : QWidget(parent), mode_(mode) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    panel_ = new Panel(mode_ == Mode::Live ? QStringLiteral("Recent detections")
                                           : QStringLiteral("Detections"),
                       this);

    auto* toolbar = new QWidget(panel_);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    searchField_ = new QLineEdit(toolbar);
    searchField_->setPlaceholderText(
        QStringLiteral("Search  process:powershell severity:high technique:T1059"));
    searchField_->setClearButtonEnabled(true);
    searchField_->setMinimumWidth(220);
    connect(searchField_, &QLineEdit::textChanged, this, &DetectionTableView::onSearchEdited);
    toolbarLayout->addWidget(searchField_, 1);

    resultCount_ = new QLabel(toolbar);
    resultCount_->setObjectName(QStringLiteral("EmptyState"));
    toolbarLayout->addWidget(resultCount_);

    severityFilter_ = new QComboBox(toolbar);
    severityFilter_->addItem(QStringLiteral("All severities"), -1);
    severityFilter_->addItem(QStringLiteral("Critical"), static_cast<int>(Severity::Critical));
    severityFilter_->addItem(QStringLiteral("High"), static_cast<int>(Severity::High));
    severityFilter_->addItem(QStringLiteral("Medium"), static_cast<int>(Severity::Medium));
    severityFilter_->addItem(QStringLiteral("Low"), static_cast<int>(Severity::Low));
    severityFilter_->addItem(QStringLiteral("Info"), static_cast<int>(Severity::Info));
    connect(severityFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { applyFilters(); });
    toolbarLayout->addWidget(severityFilter_);

    timeFilter_ = new QComboBox(toolbar);
    timeFilter_->addItem(QStringLiteral("All time"), 0);
    timeFilter_->addItem(QStringLiteral("Last 5 minutes"), 5);
    timeFilter_->addItem(QStringLiteral("Last hour"), 60);
    timeFilter_->addItem(QStringLiteral("Today"), -1);
    connect(timeFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { applyFilters(); });
    toolbarLayout->addWidget(timeFilter_);

    panel_->setHeaderAction(toolbar);

    model_ = new DetectionModel(mode_ == Mode::Live ? 50 : 10000, this);
    model_->setRelativeTimeEnabled(mode_ == Mode::Live);
    proxy_ = new DetectionFilterProxy(this);
    proxy_->setSourceModel(model_);

    auto* body = new QWidget(panel_);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);

    table_ = new QTableView(body);
    table_->setModel(proxy_);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setVisible(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSortingEnabled(true);
    table_->sortByColumn(DetectionModel::Timestamp, Qt::AscendingOrder);
    table_->verticalHeader()->setDefaultSectionSize(32);
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_->horizontalHeader()->setHighlightSections(false);
    table_->horizontalHeader()->setMinimumHeight(36);
    table_->horizontalHeader()->setSectionsClickable(true);
    table_->horizontalHeader()->setSortIndicatorShown(true);
    table_->horizontalHeader()->setSectionResizeMode(DetectionModel::Timestamp,
                                                     QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(DetectionModel::SeverityCol,
                                                     QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(DetectionModel::Technique,
                                                     QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(DetectionModel::Process,
                                                     QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(DetectionModel::Rule, QHeaderView::Stretch);
    table_->horizontalHeader()->setMinimumSectionSize(72);
    table_->setContextMenuPolicy(Qt::CustomContextMenu);

    delegate_ = new SeverityRailDelegate(table_);
    table_->setItemDelegate(delegate_);

    connect(table_, &QTableView::doubleClicked, this, &DetectionTableView::onDoubleClicked);
    connect(table_, &QTableView::activated, this, &DetectionTableView::onDoubleClicked);
    connect(table_, &QTableView::customContextMenuRequested, this,
            &DetectionTableView::onContextMenu);

    emptyLabel_ = new QLabel(table_->viewport());
    emptyLabel_->setObjectName(QStringLiteral("EmptyState"));
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setWordWrap(true);
    emptyLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    table_->viewport()->installEventFilter(this);
    table_->installEventFilter(this);

    bodyLayout->addWidget(table_, 1);
    panel_->setContent(body);
    root->addWidget(panel_, 1);

    searchDebounce_ = new QTimer(this);
    searchDebounce_->setSingleShot(true);
    searchDebounce_->setInterval(150);
    connect(searchDebounce_, &QTimer::timeout, this, &DetectionTableView::onSearchDebounce);

    auto updateCounts = [this]() {
        resultCount_->setText(QStringLiteral("%1 → %2 shown")
                                  .arg(model_->rowCount())
                                  .arg(proxy_->rowCount()));
        showEmptyState();
    };
    connect(model_, &QAbstractItemModel::rowsInserted, this, [this, updateCounts]() {
        updateCounts();
        if (mode_ == Mode::Live && model_->rowCount() > 0) {
            const auto* bar = table_->verticalScrollBar();
            if (bar->value() >= bar->maximum() - 4) {
                table_->scrollToBottom();
            }
        }
    });
    connect(model_, &QAbstractItemModel::rowsRemoved, this, updateCounts);
    connect(model_, &QAbstractItemModel::modelReset, this, updateCounts);
    connect(proxy_, &QAbstractItemModel::rowsInserted, this, updateCounts);
    connect(proxy_, &QAbstractItemModel::rowsRemoved, this, updateCounts);
    connect(proxy_, &QAbstractItemModel::modelReset, this, updateCounts);

    if (mode_ == Mode::Live) {
        relativeTimer_ = new QTimer(this);
        relativeTimer_->setInterval(1000);
        connect(relativeTimer_, &QTimer::timeout, this, &DetectionTableView::onRelativeTick);
        relativeTimer_->start();
    }

    showEmptyState();
}

void DetectionTableView::onSearchEdited(const QString&) { searchDebounce_->start(); }
void DetectionTableView::onSearchDebounce() { applyFilters(); }

void DetectionTableView::applyFilters() {
    DetectionFilterState state = parseSearchQuery(searchField_->text());
    const int sev = severityFilter_->currentData().toInt();
    if (sev >= 0) {
        state.severity = static_cast<Severity>(sev);
    }

    const int minutes = timeFilter_->currentData().toInt();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (minutes > 0) {
        state.minTimestampMs = now - static_cast<qint64>(minutes) * 60'000;
    } else if (minutes < 0) {
        state.minTimestampMs = QDateTime::currentDateTime().date().startOfDay().toMSecsSinceEpoch();
    } else {
        state.minTimestampMs = 0;
    }

    proxy_->setFilterState(state);
    resultCount_->setText(
        QStringLiteral("%1 → %2 shown").arg(model_->rowCount()).arg(proxy_->rowCount()));
    showEmptyState();
}

void DetectionTableView::setSeverityFilter(Severity severity, bool enabled) {
    if (!enabled) {
        severityFilter_->setCurrentIndex(0);
    } else {
        const int idx = severityFilter_->findData(static_cast<int>(severity));
        if (idx >= 0) {
            severityFilter_->setCurrentIndex(idx);
        }
    }
}

void DetectionTableView::setTimeRangeMinutes(int minutes) {
    const int idx = timeFilter_->findData(minutes);
    if (idx >= 0) {
        timeFilter_->setCurrentIndex(idx);
    }
}

void DetectionTableView::focusSearch() {
    searchField_->setFocus(Qt::ShortcutFocusReason);
    searchField_->selectAll();
}

void DetectionTableView::clearSearch() {
    searchField_->clear();
    applyFilters();
}

void DetectionTableView::selectRelative(int delta) {
    if (proxy_->rowCount() == 0) {
        return;
    }
    const QModelIndex cur = table_->currentIndex();
    int row = cur.isValid() ? cur.row() : (delta > 0 ? -1 : proxy_->rowCount());
    row = qBound(0, proxy_->rowCount() - 1, row + delta);
    const QModelIndex idx = proxy_->index(row, 0);
    table_->setCurrentIndex(idx);
    table_->selectRow(row);
    table_->scrollTo(idx, QAbstractItemView::EnsureVisible);
}

void DetectionTableView::selectNewest() {
    if (proxy_->rowCount() == 0) {
        return;
    }
    const int row = proxy_->rowCount() - 1;
    table_->selectRow(row);
    table_->scrollTo(proxy_->index(row, 0), QAbstractItemView::PositionAtBottom);
}

void DetectionTableView::selectOldest() {
    if (proxy_->rowCount() == 0) {
        return;
    }
    table_->selectRow(0);
    table_->scrollTo(proxy_->index(0, 0), QAbstractItemView::PositionAtTop);
}

void DetectionTableView::activateSelection() {
    const Detection* d = selectedDetection();
    if (d) {
        emit detectionActivated(*d);
    }
}

void DetectionTableView::copySelectionAsText() {
    const Detection* d = selectedDetection();
    if (d) {
        QApplication::clipboard()->setText(detectionToTsv(*d));
    }
}

void DetectionTableView::copySelectionAsJson() {
    const Detection* d = selectedDetection();
    if (d) {
        QApplication::clipboard()->setText(detectionToJson(*d));
    }
}

bool DetectionTableView::isFilteredEmpty() const {
    return model_->rowCount() > 0 && proxy_->rowCount() == 0;
}

const Detection* DetectionTableView::selectedDetection() const {
    const QModelIndex cur = table_->currentIndex();
    if (!cur.isValid()) {
        return nullptr;
    }
    return model_->at(proxy_->mapToSource(cur).row());
}

void DetectionTableView::onRelativeTick() {
    if (!model_ || model_->rowCount() == 0) {
        return;
    }
    const QModelIndex top = table_->indexAt(QPoint(0, 0));
    const QModelIndex bottom = table_->indexAt(QPoint(0, table_->viewport()->height() - 1));
    int first = top.isValid() ? proxy_->mapToSource(top).row() : 0;
    int last = bottom.isValid() ? proxy_->mapToSource(bottom).row() : model_->rowCount() - 1;
    if (!bottom.isValid()) {
        last = model_->rowCount() - 1;
    }
    model_->refreshVisibleTimestamps(first, last);
}

bool DetectionTableView::eventFilter(QObject* watched, QEvent* event) {
    if (watched == table_->viewport() && event->type() == QEvent::Resize && emptyLabel_) {
        emptyLabel_->setGeometry(table_->viewport()->rect());
    }
    if (watched == table_ && event->type() == QEvent::KeyPress) {
        const auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_J) {
            selectRelative(1);
            return true;
        }
        if (key->key() == Qt::Key_K) {
            selectRelative(-1);
            return true;
        }
        if (key->key() == Qt::Key_C && key->modifiers() == Qt::NoModifier) {
            copySelectionAsText();
            return true;
        }
        if (key->key() == Qt::Key_C && key->modifiers() == Qt::ShiftModifier) {
            copySelectionAsJson();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DetectionTableView::showEmptyState() {
    const bool pipelineEmpty = model_->rowCount() == 0;
    const bool filteredEmpty = !pipelineEmpty && proxy_->rowCount() == 0;
    emptyLabel_->setVisible(pipelineEmpty || filteredEmpty);
    if (pipelineEmpty) {
        emptyLabel_->setText(QStringLiteral(
            "No detections yet. Events will appear here as the collector processes them."));
    } else if (filteredEmpty) {
        const QString q = searchField_->text().trimmed();
        emptyLabel_->setText(q.isEmpty()
                                 ? QStringLiteral("No detections match the current filters.")
                                 : QStringLiteral("No detections match `%1`.").arg(q));
    }
    if (emptyLabel_->isVisible()) {
        emptyLabel_->setGeometry(table_->viewport()->rect());
        emptyLabel_->raise();
    }
}

void DetectionTableView::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }
    const Detection* d = model_->at(proxy_->mapToSource(index).row());
    if (d) {
        emit detectionActivated(*d);
    }
}

void DetectionTableView::onContextMenu(const QPoint& pos) {
    const QModelIndex index = table_->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    table_->setCurrentIndex(index);
    const Detection* d = selectedDetection();
    if (!d) {
        return;
    }
    const Detection copy = *d;
    QMenu menu(this);
    menu.addAction(QStringLiteral("Open Investigation"), this,
                   [this, copy]() { emit detectionActivated(copy); });
    menu.addSeparator();
    menu.addAction(QStringLiteral("Copy as text"), this, [this]() { copySelectionAsText(); });
    menu.addAction(QStringLiteral("Copy as JSON"), this, [this]() { copySelectionAsJson(); });
    menu.addAction(QStringLiteral("Copy Command Line"), this,
                   [copy]() { QApplication::clipboard()->setText(copy.commandLine); });
    menu.addAction(QStringLiteral("Copy PID"), this, [copy]() {
        QApplication::clipboard()->setText(QString::number(copy.pid));
    });
    menu.addAction(QStringLiteral("Copy Process"), this,
                   [copy]() { QApplication::clipboard()->setText(copy.processName); });
    menu.addSeparator();
    menu.addAction(QStringLiteral("Open Rule"), this,
                   [this, copy]() { emit openRuleRequested(copy.ruleId); });
    menu.addAction(QStringLiteral("Open MITRE"), this,
                   [this, copy]() { emit openMitreRequested(copy.techniqueId); });
    menu.exec(table_->viewport()->mapToGlobal(pos));
}

void DetectionTableView::onFilterChanged() { applyFilters(); }

}  // namespace sentinelforge
