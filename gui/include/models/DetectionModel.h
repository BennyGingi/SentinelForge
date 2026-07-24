#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>
#include <span>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

class DetectionModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column : int {
        Timestamp = 0,
        SeverityCol,
        Rule,
        Process,
        Technique,
        ColumnCount
    };

    enum Roles : int {
        SeverityRole = Qt::UserRole + 1,
        DetectionRole,
        TimestampMsRole,
        SearchBlobRole,
        ConfidenceRole
    };

    explicit DetectionModel(int capacity = 10000, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void appendBatch(std::span<const Detection> batch);
    const Detection* at(int row) const;
    const Detection* findById(const QString& id) const;
    void clear();

    void setRelativeTimeEnabled(bool enabled);
    bool relativeTimeEnabled() const { return relativeTimeEnabled_; }
    void refreshVisibleTimestamps(int firstRow, int lastRow);

    static QString buildSearchBlob(const Detection& d);

private:
    void evictOldest(int count);
    void indexId(const Detection& d, int row);
    void reindex();

    QVector<Detection> rows_;
    QVector<QString> searchBlobs_;
    QHash<QString, int> idToRow_;
    int capacity_;
    bool relativeTimeEnabled_ = false;
};

}  // namespace sentinelforge
