#include "models/DetectionModel.h"

#include <QDateTime>

#include "util/TimestampFormat.h"

namespace sentinelforge {

DetectionModel::DetectionModel(int capacity, QObject* parent)
    : QAbstractTableModel(parent), capacity_(capacity > 0 ? capacity : 10000) {}

QString DetectionModel::buildSearchBlob(const Detection& d) {
    QString blob;
    blob.reserve(256);
    blob += d.ruleName;
    blob += QLatin1Char(' ');
    blob += d.ruleId;
    blob += QLatin1Char(' ');
    blob += d.processName;
    blob += QLatin1Char(' ');
    blob += d.host;
    blob += QLatin1Char(' ');
    blob += d.user;
    blob += QLatin1Char(' ');
    blob += d.techniqueId;
    blob += QLatin1Char(' ');
    blob += d.commandLine;
    blob += QLatin1Char(' ');
    blob += QString::number(d.pid);
    blob += QLatin1Char(' ');
    blob += d.parentProcessName;
    for (const Ioc& ioc : d.iocs) {
        blob += QLatin1Char(' ');
        blob += iocTypeLabel(ioc.type);
        blob += QLatin1Char(' ');
        blob += ioc.value;
    }
    return blob.toLower();
}

int DetectionModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : rows_.size();
}

int DetectionModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant DetectionModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
        return {};
    }
    const Detection& d = rows_.at(index.row());

    if (role == SeverityRole) {
        return QVariant::fromValue(d.severity);
    }
    if (role == DetectionRole) {
        return QVariant::fromValue(d);
    }
    if (role == TimestampMsRole) {
        return d.timestampMs;
    }
    if (role == SearchBlobRole) {
        return searchBlobs_.at(index.row());
    }
    if (role == ConfidenceRole) {
        return d.confidence;
    }
    if (role == Qt::ToolTipRole && index.column() == Timestamp) {
        return timefmt::tooltipTime(d.timestampMs, QDateTime::currentMSecsSinceEpoch());
    }
    if (role == Qt::ToolTipRole && index.column() == Technique) {
        return d.techniqueId;
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    switch (index.column()) {
        case Timestamp: {
            QString text = timefmt::columnTime(d.timestampMs);
            if (relativeTimeEnabled_) {
                text += timefmt::relativeSuffix(d.timestampMs, QDateTime::currentMSecsSinceEpoch());
            }
            return text;
        }
        case SeverityCol:
            return severityLabel(d.severity);
        case Rule:
            return d.ruleName;
        case Process:
            return d.processName;
        case Technique:
            return d.techniqueId;
        default:
            return {};
    }
}

QVariant DetectionModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
        case Timestamp:
            return QStringLiteral("TIME");
        case SeverityCol:
            return QStringLiteral("SEVERITY");
        case Rule:
            return QStringLiteral("RULE");
        case Process:
            return QStringLiteral("PROCESS");
        case Technique:
            return QStringLiteral("TECHNIQUE");
        default:
            return {};
    }
}

void DetectionModel::appendBatch(std::span<const Detection> batch) {
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
    for (const Detection& d : batch) {
        const int row = rows_.size();
        rows_.push_back(d);
        searchBlobs_.push_back(buildSearchBlob(d));
        indexId(d, row);
    }
    endInsertRows();
}

const Detection* DetectionModel::at(int row) const {
    if (row < 0 || row >= rows_.size()) {
        return nullptr;
    }
    return &rows_.at(row);
}

const Detection* DetectionModel::findById(const QString& id) const {
    const auto it = idToRow_.constFind(id);
    if (it == idToRow_.cend()) {
        return nullptr;
    }
    return at(it.value());
}

void DetectionModel::clear() {
    if (rows_.isEmpty()) {
        return;
    }
    beginResetModel();
    rows_.clear();
    searchBlobs_.clear();
    idToRow_.clear();
    endResetModel();
}

void DetectionModel::setRelativeTimeEnabled(bool enabled) {
    if (relativeTimeEnabled_ == enabled) {
        return;
    }
    relativeTimeEnabled_ = enabled;
    if (!rows_.isEmpty()) {
        emit dataChanged(index(0, Timestamp), index(rows_.size() - 1, Timestamp),
                         {Qt::DisplayRole});
    }
}

void DetectionModel::refreshVisibleTimestamps(int firstRow, int lastRow) {
    if (!relativeTimeEnabled_ || rows_.isEmpty()) {
        return;
    }
    firstRow = qBound(0, rows_.size() - 1, firstRow);
    lastRow = qBound(0, rows_.size() - 1, lastRow);
    if (firstRow > lastRow) {
        return;
    }
    emit dataChanged(index(firstRow, Timestamp), index(lastRow, Timestamp), {Qt::DisplayRole});
}

void DetectionModel::evictOldest(int count) {
    if (count <= 0 || rows_.isEmpty()) {
        return;
    }
    count = qMin(count, rows_.size());
    beginRemoveRows({}, 0, count - 1);
    rows_.remove(0, count);
    searchBlobs_.remove(0, count);
    endRemoveRows();
    reindex();
}

void DetectionModel::indexId(const Detection& d, int row) {
    if (!d.id.isEmpty()) {
        idToRow_.insert(d.id, row);
    }
}

void DetectionModel::reindex() {
    idToRow_.clear();
    for (int i = 0; i < rows_.size(); ++i) {
        indexId(rows_.at(i), i);
    }
}

}  // namespace sentinelforge
