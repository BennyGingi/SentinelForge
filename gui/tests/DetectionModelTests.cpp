#include <gtest/gtest.h>

#include <QVector>

#include "models/DetectionModel.h"

using sentinelforge::Detection;
using sentinelforge::DetectionModel;
using sentinelforge::Severity;

namespace {
Detection makeDetection(const QString& id, qint64 ts) {
    Detection d;
    d.id = id;
    d.timestampMs = ts;
    d.severity = Severity::Info;
    d.ruleName = QStringLiteral("rule");
    d.processName = QStringLiteral("proc.exe");
    d.techniqueId = QStringLiteral("T1059");
    return d;
}

QVector<Detection> makeBatch(int count, int startIndex) {
    QVector<Detection> batch;
    batch.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int idx = startIndex + i;
        batch.push_back(makeDetection(QStringLiteral("d-%1").arg(idx), idx));
    }
    return batch;
}
}  // namespace

TEST(DetectionModelTest, AppendBatchEmitsExactlyOneInsertSignal) {
    DetectionModel model(100);
    int insertSignals = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                      [&](const QModelIndex&, int, int) { ++insertSignals; });

    model.appendBatch(makeBatch(50, 0));

    EXPECT_EQ(insertSignals, 1);
    EXPECT_EQ(model.rowCount(), 50);
}

TEST(DetectionModelTest, MultipleBatchesEachEmitExactlyOneInsertSignal) {
    DetectionModel model(1000);
    int insertSignals = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsInserted,
                      [&](const QModelIndex&, int, int) { ++insertSignals; });

    model.appendBatch(makeBatch(10, 0));
    model.appendBatch(makeBatch(10, 10));
    model.appendBatch(makeBatch(10, 20));

    EXPECT_EQ(insertSignals, 3);
    EXPECT_EQ(model.rowCount(), 30);
}

TEST(DetectionModelTest, EvictsOldestRowsAtCapacityAndReindexesLookup) {
    DetectionModel model(5);
    int removeSignals = 0;
    QObject::connect(&model, &QAbstractItemModel::rowsRemoved,
                      [&](const QModelIndex&, int, int) { ++removeSignals; });

    model.appendBatch(makeBatch(5, 0));  // d-0..d-4, fills to capacity
    ASSERT_EQ(model.rowCount(), 5);

    model.appendBatch(makeBatch(3, 5));  // d-5,d-6,d-7 -> evicts oldest 3 (d-0,d-1,d-2)
    EXPECT_EQ(model.rowCount(), 5);
    EXPECT_EQ(removeSignals, 1);

    ASSERT_NE(model.at(0), nullptr);
    EXPECT_EQ(model.at(0)->id, QStringLiteral("d-3"));
    EXPECT_EQ(model.at(4)->id, QStringLiteral("d-7"));
    EXPECT_EQ(model.findById(QStringLiteral("d-0")), nullptr);
    EXPECT_NE(model.findById(QStringLiteral("d-3")), nullptr);
}

// BRIEF §7's "bounded memory" claim, exercised at the real cap using batch
// sizes representative of the ~100ms coalescing window (small batches,
// many calls) rather than one oversized batch — see the companion test
// below for what happens when a single batch alone exceeds capacity.
TEST(DetectionModelTest, NeverExceedsDefaultCapacityOfTenThousandUnderRealisticBatching) {
    DetectionModel model;  // default capacity 10000
    for (int i = 0; i < 51; ++i) {
        model.appendBatch(makeBatch(200, i * 200));  // 51 * 200 = 10200 total events
    }
    EXPECT_EQ(model.rowCount(), 10000);
    ASSERT_NE(model.at(0), nullptr);
    EXPECT_EQ(model.at(0)->id, QStringLiteral("d-200"));  // first 200 evicted
    EXPECT_EQ(model.at(model.rowCount() - 1)->id, QStringLiteral("d-10199"));
}

// A single batch larger than capacity must still respect the cap: excess
// is trimmed from the oldest end of the incoming batch itself (after any
// existing rows are evicted), keeping only the newest capacity_ entries.
TEST(DetectionModelTest, SingleOversizedBatchRespectsCapacityCap) {
    DetectionModel model(100);
    model.appendBatch(makeBatch(150, 0));  // one batch, larger than capacity alone
    EXPECT_EQ(model.rowCount(), 100);
    ASSERT_NE(model.at(0), nullptr);
    EXPECT_EQ(model.at(0)->id, QStringLiteral("d-50"));    // oldest 50 of the batch trimmed
    EXPECT_EQ(model.at(99)->id, QStringLiteral("d-149"));  // newest entry kept
}
