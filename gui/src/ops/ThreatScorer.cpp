#include "ops/ThreatScorer.h"

#include <QDateTime>

namespace sentinelforge {

namespace {

double weight(Severity s) {
    switch (s) {
        case Severity::Critical:
            return 100.0;
        case Severity::High:
            return 25.0;
        case Severity::Medium:
            return 5.0;
        case Severity::Low:
            return 1.0;
        default:
            return 0.0;
    }
}

Severity levelFromScore(double score) {
    if (score >= 100.0) {
        return Severity::Critical;
    }
    if (score >= 40.0) {
        return Severity::High;
    }
    if (score >= 10.0) {
        return Severity::Medium;
    }
    return Severity::Low;
}

}  // namespace

ThreatScorer::ThreatScorer(QObject* parent) : QObject(parent), timer_(this) {
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &ThreatScorer::onTick);
    timer_.start();
}

void ThreatScorer::noteDetections(const QVector<Detection>& batch) {
    for (const Detection& d : batch) {
        samples_.push_back({d.timestampMs, d.severity});
    }
    onTick();
}

Severity ThreatScorer::computeRaw() const {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 window = 15 * 60 * 1000;
    double score = 0.0;
    for (const Sample& s : samples_) {
        const qint64 age = now - s.timestampMs;
        if (age < 0 || age > window) {
            continue;
        }
        const double decay = 1.0 - (static_cast<double>(age) / static_cast<double>(window));
        score += weight(s.severity) * decay;
    }
    return levelFromScore(score);
}

void ThreatScorer::onTick() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 window = 15 * 60 * 1000;
    QVector<Sample> kept;
    kept.reserve(samples_.size());
    for (const Sample& s : samples_) {
        if (now - s.timestampMs <= window) {
            kept.push_back(s);
        }
    }
    samples_.swap(kept);

    raw_ = computeRaw();
    if (raw_ > displayed_) {
        if (promoteSinceMs_ == 0) {
            promoteSinceMs_ = now;
        }
        demoteSinceMs_ = 0;
        if (now - promoteSinceMs_ >= 10'000) {
            displayed_ = raw_;
            promoteSinceMs_ = 0;
            emit levelChanged(displayed_, QStringLiteral("Threat score promoted (15m window)"));
        }
    } else if (raw_ < displayed_) {
        if (demoteSinceMs_ == 0) {
            demoteSinceMs_ = now;
        }
        promoteSinceMs_ = 0;
        if (now - demoteSinceMs_ >= 60'000) {
            displayed_ = raw_;
            demoteSinceMs_ = 0;
            emit levelChanged(displayed_, QStringLiteral("Threat score demoted (15m window)"));
        }
    } else {
        promoteSinceMs_ = 0;
        demoteSinceMs_ = 0;
    }
}

}  // namespace sentinelforge
