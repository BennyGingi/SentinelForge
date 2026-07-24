#include "telemetry/MockTelemetrySource.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <iterator>

namespace sentinelforge {

namespace {

struct SeedTemplate {
    Severity severity;
    const char* ruleId;
    const char* rule;
    const char* process;
    const char* technique;
    const char* command;
    const char* reason;
    const char* action;
    const char* parent;
};

constexpr SeedTemplate kSeedTemplates[] = {
    {Severity::Critical, "SF-T1003-001", "Suspicious LSASS access", "rundll32.exe", "T1003.001",
     "rundll32.exe C:\\Windows\\System32\\comsvcs.dll MiniDump 720 lsass.dmp full",
     "Process accessed LSASS memory with PROCESS_VM_READ after an unusual parent chain.",
     "Isolate host, dump memory, rotate credentials for interactive sessions.", "cmd.exe"},
    {Severity::High, "SF-T1059-001", "Encoded PowerShell command", "powershell.exe", "T1059.001",
     "powershell.exe -NoP -W Hidden -enc SGVsbG8gV29ybGQ=",
     "PowerShell launched with -enc and window style Hidden.",
     "Capture script block logs; review encoded payload; check lateral movement.",
     "explorer.exe"},
    {Severity::High, "SF-T1053-005", "Scheduled task created", "schtasks.exe", "T1053.005",
     "schtasks.exe /Create /TN \\Microsoft\\Windows\\Updater /TR C:\\Users\\Public\\upd.exe /SC ONLOGON",
     "New scheduled task written outside approved baselines.",
     "Disable the task, quarantine payload, review creator account.", "powershell.exe"},
    {Severity::Medium, "SF-T1547-001", "Registry run key modified", "reg.exe", "T1547.001",
     "reg.exe add HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run /v Updater /d C:\\Users\\Public\\upd.exe",
     "User-level Run key modified with a writable path payload.",
     "Remove Run key value and scan user profile for dropper.", "cmd.exe"},
    {Severity::Medium, "SF-T1543-003", "Remote service created", "sc.exe", "T1543.003",
     "sc.exe \\\\WORKSTATION-8 create BadSvc binPath= C:\\Windows\\Temp\\svc.exe",
     "Remote service creation targeting a peer workstation.",
     "Stop/delete service, collect binary, review SMB auth source.", "cmd.exe"},
    {Severity::Low, "SF-T1135", "Net share enumeration", "net.exe", "T1135",
     "net.exe view \\\\dc01",
     "Network share discovery against a domain controller.",
     "Correlate with account; unusual share enum may precede staging.", "powershell.exe"},
    {Severity::Info, "SF-PROC-CREATE", "Process created", "svchost.exe", "",
     "svchost.exe -k netsvcs -p -s Winmgmt",
     "Standard process creation telemetry.",
     "No action required unless correlated with higher-severity activity.",
     "services.exe"},
};

struct AlertTemplate {
    Severity severity;
    int confidence;
    const char* title;
    const char* description;
};

constexpr AlertTemplate kAlertTemplates[] = {
    {Severity::Critical, 94, "Credential dumping chain",
     "LSASS access followed by encoded PowerShell within 60 seconds."},
    {Severity::High, 81, "Living-off-the-land execution",
     "Signed LOLBin chain: rundll32 → powershell with encoded payload."},
    {Severity::Medium, 67, "Persistence establishment",
     "Registry run key modification followed by scheduled task creation."},
};

Severity randomSeverity() {
    const int roll = QRandomGenerator::global()->bounded(100);
    if (roll < 4) {
        return Severity::Critical;
    }
    if (roll < 18) {
        return Severity::High;
    }
    if (roll < 40) {
        return Severity::Medium;
    }
    if (roll < 70) {
        return Severity::Low;
    }
    return Severity::Info;
}

}  // namespace

MockTelemetrySource::MockTelemetrySource(QObject* parent)
    : ITelemetrySource(parent), generateTimer_(this), flushTimer_(this), statsTimer_(this) {
    generateTimer_.setInterval(250);
    flushTimer_.setInterval(100);
    statsTimer_.setInterval(1000);

    connect(&generateTimer_, &QTimer::timeout, this, &MockTelemetrySource::onGenerateTick);
    connect(&flushTimer_, &QTimer::timeout, this, &MockTelemetrySource::onFlushTick);
    connect(&statsTimer_, &QTimer::timeout, this, &MockTelemetrySource::onStatsTick);

    buildRuleCatalog();
    stats_.rulesLoaded = rules_.size();
    stats_.sigmaRulesLoaded = 8;
    stats_.correlationEnabled = true;
}

void MockTelemetrySource::buildRuleCatalog() {
    for (const SeedTemplate& tmpl : kSeedTemplates) {
        RuleInfo info;
        info.ruleId = QString::fromLatin1(tmpl.ruleId);
        info.name = QString::fromLatin1(tmpl.rule);
        info.description = QStringLiteral(
            "Detects suspicious activity matching '%1' based on process and command-line "
            "telemetry.")
                               .arg(info.name);
        info.author = QStringLiteral("SentinelForge Detection Team");
        info.version = QStringLiteral("1.2.0");
        info.sigmaSource = QStringLiteral("rules/windows/%1.yml").arg(info.ruleId.toLower());
        info.severity = tmpl.severity;
        info.techniqueId = QString::fromLatin1(tmpl.technique);
        info.logicSummary = QStringLiteral(
            "Match process Image endswith '%1' with command-line indicators from the rule "
            "selection.")
                                .arg(QString::fromLatin1(tmpl.process));
        info.knownFalsePositives =
            QStringLiteral("Legitimate admin tooling, EDR sensors, and software updates may "
                           "resemble this pattern on managed endpoints.");
        info.investigationNotes =
            QStringLiteral("Confirm parent process integrity, review recent logons on the host, "
                           "and pivot to related detections before containment.");
        rules_.insert(info.ruleId, info);
    }
}

RuleInfo MockTelemetrySource::ruleInfo(const QString& ruleId) const {
    return rules_.value(ruleId);
}

void MockTelemetrySource::start() {
    if (running_) {
        return;
    }
    running_ = true;
    emit connectionStateChanged(ConnectionState::Connected,
                                QStringLiteral("Mock telemetry source online"));
    if (!seeded_) {
        emitSeedBatch();
        seeded_ = true;
    }
    {
        QMutexLocker lock(&mutex_);
        pendingLogs_.push_back(
            makeLogLine(LogLevel::Info, QStringLiteral("MockTelemetrySource started")));
    }
    generateTimer_.start();
    flushTimer_.start();
    statsTimer_.start();
    onStatsTick();
}

void MockTelemetrySource::stop() {
    running_ = false;
    generateTimer_.stop();
    flushTimer_.stop();
    statsTimer_.stop();
    onFlushTick();
    emit connectionStateChanged(ConnectionState::Disconnected,
                                QStringLiteral("Mock telemetry source stopped"));
}

void MockTelemetrySource::emitSeedBatch() {
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QVector<Detection> detections;
    detections.reserve(40);
    for (int i = 0; i < 40; ++i) {
        const int offsetSec = (40 - i) * (15 * 60 / 40);
        detections.push_back(makeSeedDetection(i, now.addSecs(-offsetSec)));
    }

    QVector<CorrelationAlert> alerts;
    alerts.reserve(8);
    for (int i = 0; i < 8; ++i) {
        const int offsetSec = (8 - i) * (15 * 60 / 8);
        alerts.push_back(makeSeedAlert(i, now.addSecs(-offsetSec)));
    }

    {
        QMutexLocker lock(&mutex_);
        stats_.detections += static_cast<quint64>(detections.size());
        stats_.correlationAlerts += static_cast<quint64>(alerts.size());
        stats_.eventsProcessed += static_cast<quint64>(detections.size());
        pendingDetections_ += detections;
        pendingAlerts_ += alerts;
        pendingLogs_.push_back(makeLogLine(
            LogLevel::Info, QStringLiteral("Seeded 40 detections and 8 correlation alerts")));
    }
    onFlushTick();
}

void MockTelemetrySource::injectBurst(int count) {
    QMutexLocker lock(&mutex_);
    pendingDetections_.reserve(pendingDetections_.size() + count);
    for (int i = 0; i < count; ++i) {
        pendingDetections_.push_back(makeDetection());
        ++stats_.detections;
        ++stats_.eventsProcessed;
    }
    pendingLogs_.push_back(makeLogLine(
        LogLevel::Warn, QStringLiteral("Injected burst of %1 detections").arg(count)));
}

void MockTelemetrySource::onGenerateTick() {
    if (!running_) {
        return;
    }

    QMutexLocker lock(&mutex_);
    pendingDetections_.push_back(makeDetection());
    ++stats_.detections;
    ++stats_.eventsProcessed;

    if (QRandomGenerator::global()->bounded(100) < 12) {
        pendingAlerts_.push_back(makeAlert());
        ++stats_.correlationAlerts;
    }

    if (QRandomGenerator::global()->bounded(100) < 40) {
        pendingLogs_.push_back(makeLogLine(LogLevel::Info, QStringLiteral("Processed event batch")));
    }
}

void MockTelemetrySource::onFlushTick() {
    QVector<Detection> detections;
    QVector<CorrelationAlert> alerts;
    QVector<LogLine> logs;
    {
        QMutexLocker lock(&mutex_);
        detections.swap(pendingDetections_);
        alerts.swap(pendingAlerts_);
        logs.swap(pendingLogs_);
    }

    if (!detections.isEmpty()) {
        emit detectionsReceived(detections);
    }
    if (!alerts.isEmpty()) {
        emit correlationAlertsReceived(alerts);
    }
    if (!logs.isEmpty()) {
        emit logLinesReceived(logs);
    }
}

void MockTelemetrySource::onStatsTick() {
    CollectorStats snapshot;
    {
        QMutexLocker lock(&mutex_);
        stats_.cpuPercent = 8.0 + QRandomGenerator::global()->bounded(120) / 10.0;
        stats_.memoryMb = 180.0 + QRandomGenerator::global()->bounded(40);
        stats_.eventsPerSecond = 3.5 + QRandomGenerator::global()->bounded(20) / 10.0;
        stats_.pipelineLatencyMs = 2.0 + QRandomGenerator::global()->bounded(40) / 10.0;
        snapshot = stats_;
    }
    emit statsUpdated(snapshot);
}

Detection MockTelemetrySource::makeSeedDetection(int index, const QDateTime& when) {
    Severity severity;
    if (index == 0) {
        severity = Severity::Critical;
    } else if (index == 1 || index == 2) {
        severity = Severity::High;
    } else {
        severity = randomSeverity();
    }

    size_t templateIndex = 0;
    switch (severity) {
        case Severity::Critical:
            templateIndex = 0;
            break;
        case Severity::High:
            templateIndex = 1 + static_cast<size_t>(index % 2);
            break;
        case Severity::Medium:
            templateIndex = 3 + static_cast<size_t>(index % 2);
            break;
        case Severity::Low:
            templateIndex = 5;
            break;
        default:
            templateIndex = 6;
            break;
    }
    const SeedTemplate& tmpl = kSeedTemplates[templateIndex];
    ++sequence_;

    Detection d;
    d.id = QStringLiteral("seed-det-%1").arg(sequence_);
    d.timestampMs = when.toMSecsSinceEpoch();
    d.severity = severity;
    d.ruleId = QString::fromLatin1(tmpl.ruleId);
    d.ruleName = QString::fromLatin1(tmpl.rule);
    d.processName = QString::fromLatin1(tmpl.process);
    d.techniqueId = QString::fromLatin1(tmpl.technique);
    d.ruleDescription = rules_.value(d.ruleId).description;
    d.detectionReason = QString::fromLatin1(tmpl.reason);
    d.recommendedAction = QString::fromLatin1(tmpl.action);
    d.confidence = 55 + static_cast<int>(severity) * 8 + (index % 5);
    d.occurrences = 1 + (index % 4);
    d.host = QStringLiteral("WORKSTATION-%1").arg(7 + (index % 4));
    d.user = QStringLiteral("CORP\\jsmith");
    d.pid = 1200u + static_cast<quint32>(index * 17);
    d.parentPid = 600u + static_cast<quint32>(index % 9);
    d.parentProcessName = QString::fromLatin1(tmpl.parent);
    d.commandLine = QString::fromLatin1(tmpl.command);

    d.iocs = {
        {IocType::Process, d.processName},
        {IocType::Host, d.host},
        {IocType::User, d.user},
    };
    if (d.commandLine.contains(QLatin1String("C:\\"), Qt::CaseInsensitive)) {
        d.iocs.push_back({IocType::Hash, QStringLiteral("a3f2c91e8b7d4e6f0192837465abcdef01928374")});
    }
    if (severity >= Severity::High) {
        d.iocs.push_back({IocType::Ip, QStringLiteral("10.20.%1.%2")
                                           .arg(4 + (index % 3))
                                           .arg(10 + (index % 40))});
    }

    // Link to a few prior seeded ids when available (collector-owned relatedness).
    for (int i = 0; i < qMin(3, recentDetectionIds_.size()); ++i) {
        d.relatedIds.push_back(recentDetectionIds_[recentDetectionIds_.size() - 1 - i]);
    }
    recentDetectionIds_.push_back(d.id);
    while (recentDetectionIds_.size() > 32) {
        recentDetectionIds_.removeFirst();
    }

    d.rawEventJson = makeRawJson(d);
    return d;
}

Detection MockTelemetrySource::makeDetection() {
    return makeSeedDetection(static_cast<int>(sequence_ % std::size(kSeedTemplates)),
                             QDateTime::currentDateTimeUtc());
}

CorrelationAlert MockTelemetrySource::makeSeedAlert(int index, const QDateTime& when) {
    const AlertTemplate& tmpl =
        kAlertTemplates[static_cast<size_t>(index % static_cast<int>(std::size(kAlertTemplates)))];
    ++sequence_;

    CorrelationAlert a;
    a.id = QStringLiteral("seed-corr-%1").arg(sequence_);
    a.title = QString::fromLatin1(tmpl.title);
    a.description = QString::fromLatin1(tmpl.description);
    a.severity = tmpl.severity;
    a.confidence = tmpl.confidence;
    a.timestampMs = when.toMSecsSinceEpoch();
    a.matchedEventCount = 2 + (index % 3);
    a.mitreTechniques = {QStringLiteral("T1059.001"), QStringLiteral("T1003.001")};
    return a;
}

CorrelationAlert MockTelemetrySource::makeAlert() {
    return makeSeedAlert(static_cast<int>(sequence_ % std::size(kAlertTemplates)),
                         QDateTime::currentDateTimeUtc());
}

LogLine MockTelemetrySource::makeLogLine(LogLevel level, const QString& message) {
    LogLine line;
    line.timestampMs = QDateTime::currentMSecsSinceEpoch();
    line.level = level;
    line.component = QStringLiteral("MockTelemetry");
    line.message = message;
    return line;
}

QString MockTelemetrySource::makeRawJson(const Detection& d) const {
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), d.id);
    obj.insert(QStringLiteral("timestampMs"), d.timestampMs);
    obj.insert(QStringLiteral("severity"), severityLabel(d.severity));
    obj.insert(QStringLiteral("ruleId"), d.ruleId);
    obj.insert(QStringLiteral("ruleName"), d.ruleName);
    obj.insert(QStringLiteral("processName"), d.processName);
    obj.insert(QStringLiteral("techniqueId"), d.techniqueId);
    obj.insert(QStringLiteral("host"), d.host);
    obj.insert(QStringLiteral("user"), d.user);
    obj.insert(QStringLiteral("pid"), static_cast<int>(d.pid));
    obj.insert(QStringLiteral("parentPid"), static_cast<int>(d.parentPid));
    obj.insert(QStringLiteral("parentProcessName"), d.parentProcessName);
    obj.insert(QStringLiteral("commandLine"), d.commandLine);
    obj.insert(QStringLiteral("detectionReason"), d.detectionReason);
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

}  // namespace sentinelforge
