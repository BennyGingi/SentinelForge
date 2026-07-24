#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <span>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

class CorrelationModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column : int {
        Timestamp = 0,
        SeverityCol,
        Title,
        Confidence,
        Events,
        ColumnCount
    };

    enum Roles : int {
        SeverityRole = Qt::UserRole + 1,
        ConfidenceRole,
        AlertRole
    };

    explicit CorrelationModel(int capacity = 5000, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void appendBatch(std::span<const CorrelationAlert> batch);
    const CorrelationAlert* at(int row) const;
    void clear();

private:
    void evictOldest(int count);

    QVector<CorrelationAlert> rows_;
    int capacity_;
};

}  // namespace sentinelforge
