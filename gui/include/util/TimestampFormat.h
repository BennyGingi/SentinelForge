#pragma once

#include <QDateTime>
#include <QDate>
#include <QString>
#include <QTimeZone>

namespace sentinelforge::timefmt {

// Column display: absolute monospace time, always.
inline QString columnTime(qint64 timestampMs) {
    const QDateTime utc = QDateTime::fromMSecsSinceEpoch(timestampMs, QTimeZone::UTC);
    return utc.toLocalTime().toString(QStringLiteral("HH:mm:ss.zzz"));
}

// Tooltip: full context on one line.
inline QString tooltipTime(qint64 timestampMs, qint64 nowMs) {
    const QDateTime utc = QDateTime::fromMSecsSinceEpoch(timestampMs, QTimeZone::UTC);
    const QDateTime local = utc.toLocalTime();
    const qint64 ageMs = qMax<qint64>(0, nowMs - timestampMs);
    QString relative;
    if (ageMs < 1000) {
        relative = QStringLiteral("just now");
    } else if (ageMs < 60'000) {
        relative = QStringLiteral("%1 sec ago").arg(ageMs / 1000);
    } else if (ageMs < 3'600'000) {
        relative = QStringLiteral("%1 min ago").arg(ageMs / 60'000);
    } else {
        relative = QStringLiteral("%1 hr ago").arg(ageMs / 3'600'000);
    }
    const QString offset = local.timeZoneAbbreviation();
    return QStringLiteral("%1 · %2 %3 · %4")
        .arg(local.toString(QStringLiteral("dddd, d MMMM yyyy")),
             local.toString(QStringLiteral("HH:mm:ss.zzz")), offset, relative);
}

// Live dashboard only: muted relative suffix under 60s.
inline QString relativeSuffix(qint64 timestampMs, qint64 nowMs) {
    const qint64 ageMs = nowMs - timestampMs;
    if (ageMs < 0) {
        return {};
    }
    if (ageMs < 1000) {
        return QStringLiteral(" · now");
    }
    if (ageMs < 60'000) {
        return QStringLiteral(" · %1s ago").arg(ageMs / 1000);
    }
    if (ageMs < 3'600'000) {
        return QStringLiteral(" · %1 min ago").arg(ageMs / 60'000);
    }
    const QDateTime local = QDateTime::fromMSecsSinceEpoch(timestampMs, QTimeZone::UTC).toLocalTime();
    const QDate today = QDateTime::fromMSecsSinceEpoch(nowMs, QTimeZone::UTC).toLocalTime().date();
    if (local.date() == today) {
        return QStringLiteral(" · today");
    }
    if (local.date() == today.addDays(-1)) {
        return QStringLiteral(" · yesterday");
    }
    return {};
}

inline QString logClock(qint64 timestampMs) {
    return QDateTime::fromMSecsSinceEpoch(timestampMs, QTimeZone::UTC)
        .toLocalTime()
        .toString(QStringLiteral("HH:mm:ss.zzz"));
}

}  // namespace sentinelforge::timefmt
