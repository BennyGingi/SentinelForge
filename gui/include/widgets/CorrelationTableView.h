#pragma once

#include <QWidget>

#include "models/CorrelationModel.h"
#include "telemetry/TelemetryTypes.h"

class QTableView;
class QLabel;

namespace sentinelforge {

class CorrelationDelegate;
class Panel;

class CorrelationTableView final : public QWidget {
    Q_OBJECT
public:
    explicit CorrelationTableView(QWidget* parent = nullptr);
    CorrelationModel* model() const { return model_; }

signals:
    void alertActivated(sentinelforge::CorrelationAlert alert);

private:
    void showEmptyState(bool empty);
    bool eventFilter(QObject* watched, QEvent* event) override;

    Panel* panel_ = nullptr;
    CorrelationModel* model_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* emptyLabel_ = nullptr;
    CorrelationDelegate* delegate_ = nullptr;
};

}  // namespace sentinelforge
