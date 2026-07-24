#include <gtest/gtest.h>

#include <QDateTime>

#include "util/TimestampFormat.h"

using namespace sentinelforge::timefmt;

namespace {
// Build an epoch-ms value from explicit local wall-clock components so
// assertions don't depend on the test machine's timezone offset.
qint64 localMs(int y, int mo, int d, int h, int mi, int s, int ms = 0) {
    return QDateTime(QDate(y, mo, d), QTime(h, mi, s, ms), Qt::LocalTime).toMSecsSinceEpoch();
}
}  // namespace

TEST(TimestampFormatTest, ColumnTimeFormatsHhMmSsZzz) {
    const qint64 ts = localMs(2024, 3, 15, 14, 22, 7, 412);
    EXPECT_EQ(columnTime(ts), QStringLiteral("14:22:07.412"));
}

TEST(TimestampFormatTest, LogClockMatchesColumnTimeFormat) {
    const qint64 ts = localMs(2024, 3, 15, 9, 5, 0, 0);
    EXPECT_EQ(logClock(ts), QStringLiteral("09:05:00.000"));
}

TEST(TimestampFormatTest, RelativeSuffixUnderOneSecondIsNow) {
    const qint64 now = localMs(2024, 3, 15, 12, 0, 0, 500);
    const qint64 ts = localMs(2024, 3, 15, 12, 0, 0, 0);
    EXPECT_EQ(relativeSuffix(ts, now), QStringLiteral(" · now"));
}

TEST(TimestampFormatTest, RelativeSuffixUnderOneMinuteIsSeconds) {
    const qint64 now = localMs(2024, 3, 15, 12, 0, 30, 0);
    const qint64 ts = localMs(2024, 3, 15, 12, 0, 0, 0);
    EXPECT_EQ(relativeSuffix(ts, now), QStringLiteral(" · 30s ago"));
}

TEST(TimestampFormatTest, RelativeSuffixUnderOneHourIsMinutes) {
    const qint64 now = localMs(2024, 3, 15, 12, 10, 0, 0);
    const qint64 ts = localMs(2024, 3, 15, 12, 0, 0, 0);
    EXPECT_EQ(relativeSuffix(ts, now), QStringLiteral(" · 10 min ago"));
}

TEST(TimestampFormatTest, RelativeSuffixSameLocalDayIsToday) {
    const qint64 now = localMs(2024, 3, 15, 15, 0, 0, 0);
    const qint64 ts = localMs(2024, 3, 15, 13, 0, 0, 0);  // 2h earlier, same day, past the 1h boundary
    EXPECT_EQ(relativeSuffix(ts, now), QStringLiteral(" · today"));
}

TEST(TimestampFormatTest, RelativeSuffixPriorLocalDayIsYesterday) {
    const qint64 now = localMs(2024, 3, 15, 15, 0, 0, 0);
    const qint64 ts = localMs(2024, 3, 14, 15, 0, 0, 0);  // exactly 24h earlier
    EXPECT_EQ(relativeSuffix(ts, now), QStringLiteral(" · yesterday"));
}

TEST(TimestampFormatTest, RelativeSuffixOlderThanYesterdayIsEmpty) {
    const qint64 now = localMs(2024, 3, 15, 15, 0, 0, 0);
    const qint64 ts = localMs(2024, 3, 10, 15, 0, 0, 0);
    EXPECT_TRUE(relativeSuffix(ts, now).isEmpty());
}

TEST(TimestampFormatTest, RelativeSuffixFutureTimestampIsEmpty) {
    const qint64 now = localMs(2024, 3, 15, 12, 0, 0, 0);
    const qint64 ts = localMs(2024, 3, 15, 12, 0, 1, 0);  // 1s in the future
    EXPECT_TRUE(relativeSuffix(ts, now).isEmpty());
}

TEST(TimestampFormatTest, TooltipTimeContainsAbsoluteAndRelativeParts) {
    const qint64 now = localMs(2024, 3, 15, 12, 0, 5, 0);
    const qint64 ts = localMs(2024, 3, 15, 12, 0, 0, 0);
    const QString tip = tooltipTime(ts, now);
    EXPECT_TRUE(tip.contains(QStringLiteral("12:00:00.000")));
    EXPECT_TRUE(tip.contains(QStringLiteral("5 sec ago")));
}
