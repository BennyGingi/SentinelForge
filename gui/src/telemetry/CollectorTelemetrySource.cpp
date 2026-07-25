#include "telemetry/CollectorTelemetrySource.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace sentinelforge {

namespace {

QString JsonString(const QJsonObject& obj, const char* key) {
    return obj.value(QLatin1String(key)).toString();
}

}  // namespace

CollectorTelemetrySource::CollectorTelemetrySource(QUrl baseUrl, QObject* parent)
    : ITelemetrySource(parent),
      baseUrl_(std::move(baseUrl)),
      networkManager_(new QNetworkAccessManager(this)),
      pollTimer_(this) {
    pollTimer_.setInterval(kPollIntervalMs);
    connect(&pollTimer_, &QTimer::timeout, this, &CollectorTelemetrySource::poll);
}

void CollectorTelemetrySource::start() {
    if (running_) {
        return;
    }
    running_ = true;
    // First tick fires immediately rather than waiting kPollIntervalMs, so
    // the dashboard populates as soon as the source starts, not half a
    // second later.
    poll();
    pollTimer_.start();
}

void CollectorTelemetrySource::stop() {
    running_ = false;
    pollTimer_.stop();
}

RuleInfo CollectorTelemetrySource::ruleInfo(const QString& ruleId) const {
    RuleInfo info;
    info.ruleId = ruleId;
    return info;
}

void CollectorTelemetrySource::poll() {
    tickRepliesPending_ = 4;
    tickHadFailure_ = false;
    pollDetections();
    pollCorrelations();
    pollLogs();
    pollStats();
}

void CollectorTelemetrySource::pollDetections(bool isTickRequest) {
    QUrl url = baseUrl_;
    url.setPath(QStringLiteral("/detections"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("since"), QString::number(detectionCursor_));
    url.setQuery(query);

    QNetworkReply* reply = networkManager_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, isTickRequest]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            onRequestOutcome(false, isTickRequest);
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        detectionCursor_ = static_cast<quint64>(root.value(QStringLiteral("cursor")).toDouble());

        const QJsonArray items = root.value(QStringLiteral("items")).toArray();
        QVector<Detection> batch;
        batch.reserve(items.size());
        for (const QJsonValue& value : items) {
            const QJsonObject obj = value.toObject();
            Detection d;
            d.id = JsonString(obj, "id");
            d.timestampMs =
                static_cast<qint64>(obj.value(QStringLiteral("timestamp_ms")).toDouble());
            d.severity = parseSeverity(JsonString(obj, "severity"));
            d.ruleId = JsonString(obj, "rule_name");  // no rule id in the API -- see ruleInfo()
            d.ruleName = JsonString(obj, "rule_name");
            d.processName = JsonString(obj, "process_name");
            d.techniqueId = JsonString(obj, "mitre");
            d.detectionReason = JsonString(obj, "reason");
            d.host = JsonString(obj, "host");
            d.user = JsonString(obj, "user");
            d.pid = static_cast<quint32>(obj.value(QStringLiteral("pid")).toInt());
            d.parentPid = static_cast<quint32>(obj.value(QStringLiteral("parent_pid")).toInt());
            d.parentProcessName = JsonString(obj, "parent_process");
            d.commandLine = JsonString(obj, "command_line");
            // ruleDescription, recommendedAction, confidence, occurrences,
            // iocs, relatedIds, rawEventJson: the collector API doesn't send
            // any of these yet (see docs/architecture/collector-api.md's
            // "pid"/"parent_pid are always 0" note -- same story for these).
            // Left at their struct defaults rather than fabricated.
            batch.push_back(std::move(d));
        }
        if (!batch.isEmpty()) {
            emit detectionsReceived(batch);
        }
        onRequestOutcome(true, isTickRequest);

        if (root.value(QStringLiteral("more")).toBool()) {
            pollDetections(false);  // drain the backlog now, don't wait for the next tick
        }
    });
}

void CollectorTelemetrySource::pollCorrelations(bool isTickRequest) {
    QUrl url = baseUrl_;
    url.setPath(QStringLiteral("/correlations"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("since"), QString::number(correlationCursor_));
    url.setQuery(query);

    QNetworkReply* reply = networkManager_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, isTickRequest]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            onRequestOutcome(false, isTickRequest);
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        correlationCursor_ = static_cast<quint64>(root.value(QStringLiteral("cursor")).toDouble());

        const QJsonArray items = root.value(QStringLiteral("items")).toArray();
        QVector<CorrelationAlert> batch;
        batch.reserve(items.size());
        for (const QJsonValue& value : items) {
            const QJsonObject obj = value.toObject();
            CorrelationAlert a;
            a.id = JsonString(obj, "id");
            a.title = JsonString(obj, "title");
            a.description = JsonString(obj, "description");
            a.severity = parseSeverity(JsonString(obj, "severity"));
            a.confidence = obj.value(QStringLiteral("confidence")).toInt();
            a.timestampMs =
                static_cast<qint64>(obj.value(QStringLiteral("timestamp_ms")).toDouble());
            a.matchedEventCount = obj.value(QStringLiteral("matched_event_count")).toInt();
            const QJsonArray techniques = obj.value(QStringLiteral("mitre_techniques")).toArray();
            for (const QJsonValue& t : techniques) {
                a.mitreTechniques.push_back(t.toString());
            }
            batch.push_back(std::move(a));
        }
        if (!batch.isEmpty()) {
            emit correlationAlertsReceived(batch);
        }
        onRequestOutcome(true, isTickRequest);

        if (root.value(QStringLiteral("more")).toBool()) {
            pollCorrelations(false);
        }
    });
}

void CollectorTelemetrySource::pollLogs(bool isTickRequest) {
    QUrl url = baseUrl_;
    url.setPath(QStringLiteral("/logs"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("since"), QString::number(logCursor_));
    url.setQuery(query);

    QNetworkReply* reply = networkManager_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, isTickRequest]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            onRequestOutcome(false, isTickRequest);
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        logCursor_ = static_cast<quint64>(root.value(QStringLiteral("cursor")).toDouble());

        const QJsonArray items = root.value(QStringLiteral("items")).toArray();
        QVector<LogLine> batch;
        batch.reserve(items.size());
        for (const QJsonValue& value : items) {
            const QJsonObject obj = value.toObject();
            LogLine l;
            l.timestampMs =
                static_cast<qint64>(obj.value(QStringLiteral("timestamp_ms")).toDouble());
            l.level = parseLogLevel(JsonString(obj, "level"));
            l.component = JsonString(obj, "component");
            l.message = JsonString(obj, "message");
            batch.push_back(std::move(l));
        }
        if (!batch.isEmpty()) {
            emit logLinesReceived(batch);
        }
        onRequestOutcome(true, isTickRequest);

        if (root.value(QStringLiteral("more")).toBool()) {
            pollLogs(false);
        }
    });
}

void CollectorTelemetrySource::pollStats() {
    QUrl url = baseUrl_;
    url.setPath(QStringLiteral("/stats"));

    QNetworkReply* reply = networkManager_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            onRequestOutcome(false, /*isTickRequest=*/true);
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        CollectorStats stats;
        stats.eventsProcessed =
            static_cast<quint64>(obj.value(QStringLiteral("events_processed")).toDouble());
        stats.detections = static_cast<quint64>(obj.value(QStringLiteral("detections")).toDouble());
        stats.correlationAlerts =
            static_cast<quint64>(obj.value(QStringLiteral("correlation_alerts")).toDouble());
        // StatusStrip's RULES chip computes rulesLoaded + sigmaRulesLoaded
        // (see StatusStrip.cpp) -- a convention MockTelemetrySource set by
        // treating rulesLoaded as native-only, additive with sigma. The
        // collector's /stats "rules_loaded" is the *combined* total
        // (native + sigma already merged, see Application::LoadRules), so
        // passed straight through it double-counts sigma rules in that sum.
        // Subtract sigma back out here to match the display convention the
        // GUI already has, rather than changing collector-cpp's API
        // semantics (out of phase 2's scope) or StatusStrip's math.
        const int sigmaLoaded = obj.value(QStringLiteral("sigma_rules_loaded")).toInt();
        const int totalLoaded = obj.value(QStringLiteral("rules_loaded")).toInt();
        stats.rulesLoaded = totalLoaded - sigmaLoaded;
        stats.sigmaRulesLoaded = sigmaLoaded;
        // No "correlation enabled" flag in the API -- correlation_rules_loaded
        // (a count) is the closest available signal.
        stats.correlationEnabled =
            obj.value(QStringLiteral("correlation_rules_loaded")).toInt() > 0;
        // cpuPercent/memoryMb: the collector API reports no process resource
        // usage at all (out of scope for phase 1's /stats). Left at 0.0
        // rather than fabricated -- the Performance panel's CPU/Memory
        // blocks will read 0 against a real collector until that's added.
        stats.eventsPerSecond = obj.value(QStringLiteral("events_per_second")).toDouble();
        stats.pipelineLatencyMs = obj.value(QStringLiteral("pipeline_latency_ms")).toDouble();

        emit statsUpdated(stats);
        onRequestOutcome(true, /*isTickRequest=*/true);
    });
}

void CollectorTelemetrySource::onRequestOutcome(bool succeeded, bool isTickRequest) {
    if (!isTickRequest) {
        // A "more": true continuation, draining a backlog page outside the
        // regular tick cadence. It doesn't participate in the once-per-tick
        // connection-state verdict below (see the member comment on
        // tickRepliesPending_): folding it in would let a single tick's
        // request count grow unpredictably and double-count outcomes.
        // Failure here is rare -- the network was just fine a moment ago,
        // by definition, to have received "more": true at all -- but still
        // worth reacting to immediately rather than silently swallowing it.
        if (!succeeded) {
            ++consecutiveFailedTicks_;
            applyBackoffInterval();
            if (currentState_ == ConnectionState::Connected) {
                currentState_ = ConnectionState::Reconnecting;
                emit connectionStateChanged(
                    ConnectionState::Reconnecting,
                    QStringLiteral("Lost contact with collector at %1, retrying")
                        .arg(baseUrl_.toString()));
            }
        }
        return;
    }

    tickHadFailure_ = tickHadFailure_ || !succeeded;
    if (--tickRepliesPending_ > 0) {
        return;  // wait for the rest of this tick's replies
    }

    // Every one of this tick's 4 requests has now completed -- exactly one
    // state-machine verdict for the whole tick, regardless of how many of
    // the 4 individually failed.
    if (tickHadFailure_) {
        onTickFailed();
    } else {
        onTickSucceeded();
    }
}

void CollectorTelemetrySource::onTickSucceeded() {
    consecutiveFailedTicks_ = 0;
    applyBackoffInterval();
    if (currentState_ != ConnectionState::Connected) {
        currentState_ = ConnectionState::Connected;
        emit connectionStateChanged(
            ConnectionState::Connected,
            QStringLiteral("Connected to collector at %1").arg(baseUrl_.toString()));
    }
}

void CollectorTelemetrySource::onTickFailed() {
    ++consecutiveFailedTicks_;
    applyBackoffInterval();

    if (consecutiveFailedTicks_ >= kFailedThreshold) {
        if (currentState_ != ConnectionState::Failed) {
            currentState_ = ConnectionState::Failed;
            emit connectionStateChanged(
                ConnectionState::Failed,
                QStringLiteral("%1 consecutive failed retries reaching %2 -- still retrying")
                    .arg(consecutiveFailedTicks_)
                    .arg(baseUrl_.toString()));
        }
    } else if (currentState_ != ConnectionState::Reconnecting &&
               currentState_ != ConnectionState::Failed) {
        currentState_ = ConnectionState::Reconnecting;
        emit connectionStateChanged(
            ConnectionState::Reconnecting,
            QStringLiteral("Lost contact with collector at %1, retrying").arg(baseUrl_.toString()));
    }
}

void CollectorTelemetrySource::applyBackoffInterval() {
    // Exponential backoff on repeated failed ticks, capped, reset to the
    // base interval the moment a tick fully succeeds again. running_ guard:
    // don't restart the timer (which setInterval() does, per Qt docs, when
    // the timer is already active) once stop() has been called.
    if (!running_) {
        return;
    }
    const int interval =
        consecutiveFailedTicks_ == 0
            ? kPollIntervalMs
            : std::min(kPollIntervalMs * (1 << std::min(consecutiveFailedTicks_, 4)),
                       kMaxBackoffMs);
    if (pollTimer_.interval() != interval) {
        pollTimer_.setInterval(interval);
    }
}

}  // namespace sentinelforge
