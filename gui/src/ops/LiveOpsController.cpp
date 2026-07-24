#include "ops/LiveOpsController.h"

#include <algorithm>

namespace sentinelforge {

namespace {

template <typename T>
Severity itemSeverity(const T& item) {
    return item.severity;
}

template <>
Severity itemSeverity<LogLine>(const LogLine& line) {
    switch (line.level) {
        case LogLevel::Fatal:
        case LogLevel::Error:
            return Severity::Critical;
        case LogLevel::Warn:
            return Severity::Medium;
        default:
            return Severity::Info;
    }
}

template <typename T>
bool isHighPriority(const T& item) {
    const Severity s = itemSeverity(item);
    return s == Severity::Critical || s == Severity::High;
}

}  // namespace

LiveOpsController::LiveOpsController(QObject* parent) : QObject(parent), flushTimer_(this) {
    flushTimer_.setInterval(0);
    connect(&flushTimer_, &QTimer::timeout, this, &LiveOpsController::onFlushTick);
}

int LiveOpsController::bufferedDetections() const { return detectionBuffer_.size(); }
int LiveOpsController::bufferedAlerts() const { return alertBuffer_.size(); }
int LiveOpsController::bufferedLogs() const { return logBuffer_.size(); }

void LiveOpsController::setPaused(bool paused) {
    if (paused_ == paused) {
        return;
    }
    paused_ = paused;
    if (!paused_) {
        flushTimer_.start();
    } else {
        flushTimer_.stop();
    }
    emit pauseStateChanged(paused_, bufferedDetections() + bufferedAlerts() + bufferedLogs(),
                           dropped_);
}

void LiveOpsController::togglePaused() { setPaused(!paused_); }

void LiveOpsController::ingestDetections(const QVector<Detection>& batch) {
    if (batch.isEmpty()) {
        return;
    }
    for (const Detection& d : batch) {
        if (d.severity == Severity::Critical) {
            emit criticalDetected(d);
        }
    }
    if (!paused_) {
        emit detectionsReady(batch);
        return;
    }
    dropped_ += appendWithEviction(detectionBuffer_, batch, kDetectionCap);
    emit pauseStateChanged(paused_, bufferedDetections() + bufferedAlerts() + bufferedLogs(),
                           dropped_);
}

void LiveOpsController::ingestAlerts(const QVector<CorrelationAlert>& batch) {
    if (batch.isEmpty()) {
        return;
    }
    if (!paused_) {
        emit alertsReady(batch);
        return;
    }
    dropped_ += appendWithEviction(alertBuffer_, batch, kAlertCap);
    emit pauseStateChanged(paused_, bufferedDetections() + bufferedAlerts() + bufferedLogs(),
                           dropped_);
}

void LiveOpsController::ingestLogs(const QVector<LogLine>& batch) {
    if (batch.isEmpty()) {
        return;
    }
    if (!paused_) {
        emit logsReady(batch);
        return;
    }
    dropped_ += appendWithEviction(logBuffer_, batch, kLogCap);
    emit pauseStateChanged(paused_, bufferedDetections() + bufferedAlerts() + bufferedLogs(),
                           dropped_);
}

template <typename T>
int LiveOpsController::appendWithEviction(QVector<T>& buffer, const QVector<T>& incoming, int cap) {
    buffer += incoming;
    int dropped = 0;
    while (buffer.size() > cap) {
        int dropIndex = -1;
        for (int i = 0; i < buffer.size(); ++i) {
            if (!isHighPriority(buffer.at(i))) {
                dropIndex = i;
                break;
            }
        }
        if (dropIndex < 0) {
            // Only Critical/High remain — drop oldest anyway to stay bounded.
            dropIndex = 0;
        }
        buffer.removeAt(dropIndex);
        ++dropped;
    }
    return dropped;
}

void LiveOpsController::onFlushTick() {
    if (paused_) {
        flushTimer_.stop();
        return;
    }

    bool more = false;
    if (!detectionBuffer_.isEmpty()) {
        const int n = qMin(kFlushBatch, detectionBuffer_.size());
        QVector<Detection> chunk;
        chunk.reserve(n);
        for (int i = 0; i < n; ++i) {
            chunk.push_back(detectionBuffer_.takeFirst());
        }
        emit detectionsReady(chunk);
        more = more || !detectionBuffer_.isEmpty();
    }
    if (!alertBuffer_.isEmpty()) {
        const int n = qMin(kFlushBatch, alertBuffer_.size());
        QVector<CorrelationAlert> chunk;
        chunk.reserve(n);
        for (int i = 0; i < n; ++i) {
            chunk.push_back(alertBuffer_.takeFirst());
        }
        emit alertsReady(chunk);
        more = more || !alertBuffer_.isEmpty();
    }
    if (!logBuffer_.isEmpty()) {
        const int n = qMin(kFlushBatch, logBuffer_.size());
        QVector<LogLine> chunk;
        chunk.reserve(n);
        for (int i = 0; i < n; ++i) {
            chunk.push_back(logBuffer_.takeFirst());
        }
        emit logsReady(chunk);
        more = more || !logBuffer_.isEmpty();
    }

    emit pauseStateChanged(paused_, bufferedDetections() + bufferedAlerts() + bufferedLogs(),
                           dropped_);

    if (!more) {
        flushTimer_.stop();
        dropped_ = 0;
        emit pauseStateChanged(false, 0, 0);
    }
}

}  // namespace sentinelforge
