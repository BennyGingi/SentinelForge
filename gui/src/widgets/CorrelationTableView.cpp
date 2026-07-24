#include "widgets/CorrelationTableView.h"

#include <QEvent>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

#include "widgets/CorrelationDelegate.h"
#include "widgets/Panel.h"

namespace sentinelforge {

CorrelationTableView::CorrelationTableView(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    panel_ = new Panel(QStringLiteral("Correlation alerts"), this);

    model_ = new CorrelationModel(5000, this);

    auto* body = new QWidget(panel_);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);

    table_ = new QTableView(body);
    table_->setModel(model_);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(false);
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setVisible(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setDefaultSectionSize(32);
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_->horizontalHeader()->setMinimumHeight(36);
    table_->horizontalHeader()->setSectionResizeMode(CorrelationModel::Timestamp,
                                                     QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(CorrelationModel::SeverityCol,
                                                     QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(CorrelationModel::Confidence,
                                                     QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(CorrelationModel::Title, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(CorrelationModel::Events,
                                                     QHeaderView::ResizeToContents);

    delegate_ = new CorrelationDelegate(table_);
    table_->setItemDelegate(delegate_);

    emptyLabel_ = new QLabel(
        QStringLiteral("No correlation alerts yet. Behavioral matches will appear here."),
        table_->viewport());
    emptyLabel_->setObjectName(QStringLiteral("EmptyState"));
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setWordWrap(true);
    emptyLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    table_->viewport()->installEventFilter(this);

    bodyLayout->addWidget(table_, 1);
    panel_->setContent(body);
    root->addWidget(panel_, 1);

    showEmptyState(true);

    connect(model_, &QAbstractItemModel::rowsInserted, this, [this]() {
        showEmptyState(model_->rowCount() == 0);
        if (model_->rowCount() > 0) {
            table_->scrollToBottom();
        }
    });
    connect(model_, &QAbstractItemModel::rowsRemoved, this,
            [this]() { showEmptyState(model_->rowCount() == 0); });

    connect(table_, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        const CorrelationAlert* a = model_->at(index.row());
        if (a) {
            emit alertActivated(*a);
        }
    });
}

bool CorrelationTableView::eventFilter(QObject* watched, QEvent* event) {
    if (watched == table_->viewport() && event->type() == QEvent::Resize && emptyLabel_) {
        emptyLabel_->setGeometry(table_->viewport()->rect());
    }
    return QWidget::eventFilter(watched, event);
}

void CorrelationTableView::showEmptyState(bool empty) {
    emptyLabel_->setVisible(empty);
    if (empty) {
        emptyLabel_->setGeometry(table_->viewport()->rect());
        emptyLabel_->raise();
    }
}

}  // namespace sentinelforge
