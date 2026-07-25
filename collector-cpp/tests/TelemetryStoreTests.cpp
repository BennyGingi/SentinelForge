#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "TelemetryStore.h"

namespace sentinelforge {
namespace {

StoredDetection MakeDetection(const std::string& ruleName) {
    StoredDetection d;
    d.ruleName = ruleName;
    d.severity = "High";
    d.processName = "powershell.exe";
    return d;
}

// Drains every page from `since` to the end, in order, so tests don't have
// to reason about the 500-item page cap directly.
std::vector<StoredDetection> DrainAll(const TelemetryStore& store, std::uint64_t since = 0) {
    std::vector<StoredDetection> all;
    std::uint64_t cursor = since;
    while (true) {
        const auto page = store.DetectionsSince(cursor);
        all.insert(all.end(), page.items.begin(), page.items.end());
        if (!page.more) {
            break;
        }
        cursor = page.cursor;
    }
    return all;
}

TEST(TelemetryStoreTest, SequenceIsMonotonicFromOne) {
    TelemetryStore store;
    for (int i = 0; i < 10; ++i) {
        store.AppendDetection(MakeDetection("rule-" + std::to_string(i)));
    }

    const auto page = store.DetectionsSince(0);
    ASSERT_EQ(page.items.size(), 10u);
    for (std::size_t i = 0; i < page.items.size(); ++i) {
        EXPECT_EQ(page.items[i].sequence, i + 1) << "sequence must be 1-indexed and gapless";
    }
    EXPECT_EQ(page.cursor, 10u);
    EXPECT_FALSE(page.more);
}

TEST(TelemetryStoreTest, SinceExcludesAlreadySeenItems) {
    TelemetryStore store;
    for (int i = 0; i < 5; ++i) {
        store.AppendDetection(MakeDetection("rule"));
    }

    const auto page = store.DetectionsSince(3);
    ASSERT_EQ(page.items.size(), 2u);
    EXPECT_EQ(page.items.front().sequence, 4u);
    EXPECT_EQ(page.items.back().sequence, 5u);
    EXPECT_EQ(page.cursor, 5u);
}

TEST(TelemetryStoreTest, SinceAtOrPastHeadReturnsEmptyWithCurrentCursor) {
    TelemetryStore store;
    for (int i = 0; i < 3; ++i) {
        store.AppendDetection(MakeDetection("rule"));
    }

    const auto page = store.DetectionsSince(3);
    EXPECT_TRUE(page.items.empty());
    EXPECT_EQ(page.cursor, 3u) << "an already-caught-up client should get the current head back, "
                                  "not a stale/zero cursor";
    EXPECT_FALSE(page.more);
}

TEST(TelemetryStoreTest, EvictsOldestAtDetectionCap) {
    TelemetryStore store;
    const std::size_t total = TelemetryStore::kDetectionCap + 250;
    for (std::size_t i = 0; i < total; ++i) {
        store.AppendDetection(MakeDetection("rule-" + std::to_string(i)));
    }

    const auto retained = DrainAll(store);
    ASSERT_EQ(retained.size(), TelemetryStore::kDetectionCap);
    // The oldest 250 (sequence 1..250) must be gone; the retained range is
    // contiguous and ends at the last-written sequence.
    EXPECT_EQ(retained.front().sequence, 251u);
    EXPECT_EQ(retained.back().sequence, total);
    for (std::size_t i = 1; i < retained.size(); ++i) {
        EXPECT_EQ(retained[i].sequence, retained[i - 1].sequence + 1)
            << "retained sequence range must stay contiguous after eviction";
    }
}

TEST(TelemetryStoreTest, EvictsOldestAtAlertAndLogCaps) {
    TelemetryStore store;
    for (std::size_t i = 0; i < TelemetryStore::kAlertCap + 100; ++i) {
        StoredCorrelationAlert a;
        a.title = "alert";
        store.AppendAlert(a);
    }
    for (std::size_t i = 0; i < TelemetryStore::kLogCap + 100; ++i) {
        StoredLogLine l;
        l.message = "log";
        store.AppendLog(l);
    }

    std::uint64_t alertCursor = 0;
    std::size_t alertCount = 0;
    while (true) {
        const auto page = store.AlertsSince(alertCursor);
        alertCount += page.items.size();
        if (!page.more)
            break;
        alertCursor = page.cursor;
    }
    EXPECT_EQ(alertCount, TelemetryStore::kAlertCap);

    std::uint64_t logCursor = 0;
    std::size_t logCount = 0;
    while (true) {
        const auto page = store.LogsSince(logCursor);
        logCount += page.items.size();
        if (!page.more)
            break;
        logCursor = page.cursor;
    }
    EXPECT_EQ(logCount, TelemetryStore::kLogCap);
}

TEST(TelemetryStoreTest, SincePastEvictedWindowReturnsWhatsAvailableNotAnError) {
    TelemetryStore store;
    const std::size_t total = TelemetryStore::kDetectionCap + 500;
    for (std::size_t i = 0; i < total; ++i) {
        store.AppendDetection(MakeDetection("rule"));
    }

    // Sequence 1 was evicted long ago -- querying since=1 must not crash,
    // error, or return a bogus/negative range. It should just behave as
    // "give me everything you still have."
    const auto page = store.DetectionsSince(1);
    EXPECT_EQ(page.items.size(), TelemetryStore::kMaxPageSize);
    EXPECT_TRUE(page.more);
    EXPECT_EQ(page.items.front().sequence, total - TelemetryStore::kDetectionCap + 1);

    const auto all = DrainAll(store, 1);
    EXPECT_EQ(all.size(), TelemetryStore::kDetectionCap)
        << "since=1 must yield exactly the retained window, no more, no less, no crash";
}

TEST(TelemetryStoreTest, PageCapsAt500AndCursorContinuesFromLastReturnedItem) {
    TelemetryStore store;
    for (std::size_t i = 0; i < 1200; ++i) {
        store.AppendDetection(MakeDetection("rule"));
    }

    const auto first = store.DetectionsSince(0);
    EXPECT_EQ(first.items.size(), TelemetryStore::kMaxPageSize);
    EXPECT_TRUE(first.more);
    EXPECT_EQ(first.cursor, 500u) << "cursor must be the last item in THIS page, not the head, "
                                     "or a paginating client skips undelivered items";

    const auto second = store.DetectionsSince(first.cursor);
    EXPECT_EQ(second.items.front().sequence, 501u);
}

TEST(TelemetryStoreTest, ConcurrentWriteWhileReadDoesNotCorruptOrCrash) {
    TelemetryStore store;
    // Stays under kDetectionCap deliberately: eviction is already covered by
    // EvictsOldestAtDetectionCap above, and mixing it in here would make
    // "no writes lost" an invalid assertion instead of a meaningful one.
    constexpr int kWrites = 5000;
    std::atomic<bool> stopReading{false};
    std::atomic<int> readCount{0};

    std::thread writer([&store] {
        for (int i = 0; i < kWrites; ++i) {
            store.AppendDetection(MakeDetection("rule"));
        }
    });

    // Concurrently hammer reads from multiple cursors while the writer is
    // still going. The assertion here is simply "this doesn't crash, hang,
    // or throw" -- correctness of the final state is checked after both
    // threads finish.
    std::thread reader([&] {
        std::uint64_t cursor = 0;
        while (!stopReading.load()) {
            const auto page = store.DetectionsSince(cursor);
            cursor = page.cursor;
            ++readCount;
            (void)store.Stats();
            std::this_thread::yield();  // don't starve the writer via lock contention
        }
    });

    writer.join();
    stopReading.store(true);
    reader.join();

    EXPECT_GT(readCount.load(), 0);

    const auto all = DrainAll(store);
    EXPECT_EQ(all.size(), static_cast<std::size_t>(kWrites))
        << "no writes lost or duplicated under concurrent read access";
    for (std::size_t i = 1; i < all.size(); ++i) {
        ASSERT_EQ(all[i].sequence, all[i - 1].sequence + 1)
            << "no torn/duplicated/reordered entries under concurrent access";
    }
}

TEST(TelemetryStoreTest, StatsReflectAppendedCountsAndRuleLoadCounts) {
    TelemetryStore store;
    store.AppendDetection(MakeDetection("a"));
    store.AppendDetection(MakeDetection("b"));
    StoredCorrelationAlert alert;
    store.AppendAlert(alert);
    store.SetRulesLoaded(7, 3, 1);

    const TelemetryStats stats = store.Stats();
    EXPECT_EQ(stats.detections, 2u);
    EXPECT_EQ(stats.correlationAlerts, 1u);
    EXPECT_EQ(stats.rulesLoaded, 7);
    EXPECT_EQ(stats.sigmaRulesLoaded, 3);
    EXPECT_EQ(stats.correlationRulesLoaded, 1);
}

TEST(TelemetryStoreTest, GeneratedIdsAreStableAndUnique) {
    TelemetryStore store;
    store.AppendDetection(MakeDetection("a"));
    store.AppendDetection(MakeDetection("b"));

    const auto page = store.DetectionsSince(0);
    ASSERT_EQ(page.items.size(), 2u);
    EXPECT_NE(page.items[0].id, page.items[1].id);
    EXPECT_FALSE(page.items[0].id.empty());
}

TEST(TelemetryStoreTest, ParseIso8601ToEpochMsHandlesValidAndInvalidInput) {
    // Verified against Python's calendar/datetime (independent oracle),
    // not hand-computed -- a hand-computed expectation here would just be
    // testing the parser against my own arithmetic mistakes.
    EXPECT_EQ(TelemetryStore::ParseIso8601ToEpochMs("2026-07-20T14:32:07Z"), 1784557927000LL);
    EXPECT_EQ(TelemetryStore::ParseIso8601ToEpochMs("2026-07-20T14:32:07.412Z"), 1784557927412LL);
    EXPECT_EQ(TelemetryStore::ParseIso8601ToEpochMs(""), 0);
    EXPECT_EQ(TelemetryStore::ParseIso8601ToEpochMs("not a timestamp"), 0);
    EXPECT_EQ(TelemetryStore::ParseIso8601ToEpochMs("2026-13-40T99:99:99Z"), 0)
        << "out-of-range fields must be rejected, not silently normalized/rolled-over by "
           "timegm into a plausible-looking but wrong epoch value";
}

}  // namespace
}  // namespace sentinelforge
