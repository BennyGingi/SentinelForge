#include <gtest/gtest.h>

#include <QVector>

#include "models/CorrelationModel.h"

using sentinelforge::CorrelationAlert;
using sentinelforge::CorrelationModel;
using sentinelforge::Severity;

namespace {
CorrelationAlert makeAlert(const QString& id, qint64 ts) {
    CorrelationAlert a;
    a.id = id;
    a.timestampMs = ts;
    a.severity = Severity::Info;
    a.title = QStringLiteral("alert");
    a.confidence = 50;
    a.matchedEventCount = 2;
    return a;
}

QVector<CorrelationAlert> makeBatch(int count, int startIndex) {
    QVector<CorrelationAlert> batch;
    batch.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int idx = startIndex + i;
        batch.push_back(makeAlert(QStringLiteral("a-%1").arg(idx), idx));
    }
    return batch;
}
}  // namespace

TEST(CorrelationModelTest, AppendBatchEmitsExactlyOneInsertSignal) {
    CorrelationModel model(100);
    int insertSignals = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                      [&](const QModelIndex&, int, int) { ++insertSignals; });

    model.appendBatch(makeBatch(20, 0));

    EXPECT_EQ(insertSignals, 1);
    EXPECT_EQ(model.rowCount(), 20);
}

TEST(CorrelationModelTest, MultipleBatchesEachEmitExactlyOneInsertSignal) {
    CorrelationModel model(1000);
    int insertSignals = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                      [&](const QModelIndex&, int, int) { ++insertSignals; });

    model.appendBatch(makeBatch(5, 0));
    model.appendBatch(makeBatch(5, 5));

    EXPECT_EQ(insertSignals, 2);
    EXPECT_EQ(model.rowCount(), 10);
}

TEST(CorrelationModelTest, EvictsOldestRowsAtCapacity) {
    CorrelationModel model(5);
    int removeSignals = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsRemoved,
                      [&](const QModelIndex&, int, int) { ++removeSignals; });

    model.appendBatch(makeBatch(5, 0));  // a-0..a-4, fills to capacity
    ASSERT_EQ(model.rowCount(), 5);

    model.appendBatch(makeBatch(3, 5));  // a-5,a-6,a-7 -> evicts oldest 3 (a-0,a-1,a-2)
    EXPECT_EQ(model.rowCount(), 5);
    EXPECT_EQ(removeSignals, 1);

    ASSERT_NE(model.at(0), nullptr);
    EXPECT_EQ(model.at(0)->id, QStringLiteral("a-3"));
    EXPECT_EQ(model.at(4)->id, QStringLiteral("a-7"));
}

// BRIEF §7's "bounded memory" claim for correlation, exercised at the real
// 5,000 cap using batch sizes representative of the coalescing window.
TEST(CorrelationModelTest, NeverExceedsDefaultCapacityOfFiveThousandUnderRealisticBatching) {
    CorrelationModel model;  // default capacity 5000
    for (int i = 0; i < 26; ++i) {
        model.appendBatch(makeBatch(200, i * 200));  // 26 * 200 = 5200 total alerts
    }
    EXPECT_EQ(model.rowCount(), 5000);
    ASSERT_NE(model.at(0), nullptr);
    EXPECT_EQ(model.at(0)->id, QStringLiteral("a-200"));  // first 200 evicted
    EXPECT_EQ(model.at(model.rowCount() - 1)->id, QStringLiteral("a-5199"));
}

// A single batch larger than capacity must still respect the cap: excess
// is trimmed from the oldest end of the incoming batch itself (after any
// existing rows are evicted), keeping only the newest capacity_ entries.
TEST(CorrelationModelTest, SingleOversizedBatchRespectsCapacityCap) {
    CorrelationModel model(100);
    model.appendBatch(makeBatch(150, 0));
    EXPECT_EQ(model.rowCount(), 100);
    ASSERT_NE(model.at(0), nullptr);
    EXPECT_EQ(model.at(0)->id, QStringLiteral("a-50"));    // oldest 50 of the batch trimmed
    EXPECT_EQ(model.at(99)->id, QStringLiteral("a-149"));  // newest entry kept
}
