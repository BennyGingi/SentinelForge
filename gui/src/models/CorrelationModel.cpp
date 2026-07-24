#include "models/CorrelationModel.h"

#include "util/TimestampFormat.h"

namespace sentinelforge {

CorrelationModel::CorrelationModel(int capacity, QObject* parent)
    : QAbstractTableModel(parent), capacity_(capacity > 0 ? capacity : 5000) {}

int CorrelationModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : rows_.size();
}

int CorrelationModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant CorrelationModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
        return {};
    }
    const CorrelationAlert& a = rows_.at(index.row());

    if (role == SeverityRole) {
        return QVariant::fromValue(a.severity);
    }
    if (role == ConfidenceRole) {
        return a.confidence;
    }
    if (role == AlertRole) {
        return QVariant::fromValue(a);
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    switch (index.column()) {
        case Timestamp:
            return timefmt::columnTime(a.timestampMs);
        case SeverityCol:
            return severityLabel(a.severity);
        case Title:
            return a.title;
        case Confidence:
            return QStringLiteral("%1%").arg(a.confidence);
        case Events:
            return a.matchedEventCount;
        default:
            return {};
    }
}

QVariant CorrelationModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
        case Timestamp:
            return QStringLiteral("TIME");
        case SeverityCol:
            return QStringLiteral("SEVERITY");
        case Title:
            return QStringLiteral("BEHAVIOR");
        case Confidence:
            return QStringLiteral("CONFIDENCE");
        case Events:
            return QStringLiteral("EVENTS");
        default:
            return {};
    }
}

void CorrelationModel::appendBatch(std::span<const CorrelationAlert> batch) {
    if (batch.empty()) {
        return;
    }

    const int overflow = rows_.size() + static_cast<int>(batch.size()) - capacity_;
    if (overflow > 0) {
        evictOldest(overflow);
    }

    const int first = rows_.size();
    const int last = first + static_cast<int>(batch.size()) - 1;
    beginInsertRows({}, first, last);
    for (const CorrelationAlert& a : batch) {
        rows_.push_back(a);
    }
    endInsertRows();
}

const CorrelationAlert* CorrelationModel::at(int row) const {
    if (row < 0 || row >= rows_.size()) {
        return nullptr;
    }
    return &rows_.at(row);
}

void CorrelationModel::clear() {
    if (rows_.isEmpty()) {
        return;
    }
    beginResetModel();
    rows_.clear();
    endResetModel();
}

void CorrelationModel::evictOldest(int count) {
    if (count <= 0 || rows_.isEmpty()) {
        return;
    }
    count = qMin(count, rows_.size());
    beginRemoveRows({}, 0, count - 1);
    rows_.remove(0, count);
    endRemoveRows();
}

}  // namespace sentinelforge
