#pragma once

#include <QSortFilterProxyModel>
#include <QString>
#include <QVector>
#include <optional>

#include "telemetry/TelemetryTypes.h"

namespace sentinelforge {

struct SearchToken {
    QString field;  // empty = bare term
    QString value;
};

struct DetectionFilterState {
    QVector<SearchToken> tokens;
    std::optional<Severity> severity;
    QString host;
    QString process;
    QString rule;
    QString technique;
    qint64 minTimestampMs = 0;  // 0 = no lower bound
};

DetectionFilterState parseSearchQuery(const QString& query);

class DetectionFilterProxy final : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit DetectionFilterProxy(QObject* parent = nullptr);

    void setFilterState(DetectionFilterState state);
    const DetectionFilterState& filterState() const { return state_; }
    QString rawQuery() const { return rawQuery_; }
    void setRawQuery(const QString& query);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    bool matchesTokens(const QModelIndex& srcIndex) const;

    DetectionFilterState state_;
    QString rawQuery_;
};

}  // namespace sentinelforge
