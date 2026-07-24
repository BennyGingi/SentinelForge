#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace sentinelforge {

enum class Severity : quint8 {
    Info = 0,
    Low,
    Medium,
    High,
    Critical
};

enum class ConnectionState : quint8 {
    Disconnected = 0,
    Connecting,
    Connected,
    Reconnecting,
    Failed
};

enum class LogLevel : quint8 {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

enum class IocType : quint8 {
    Process = 0,
    User,
    Host,
    Ip,
    Domain,
    Url,
    RegistryKey,
    Hash,
    Service,
    ScheduledTask
};

struct Ioc {
    IocType type = IocType::Process;
    QString value;
};

// Populated entirely by the telemetry source. GUI never extracts IOCs or
// related-event links from rawEventJson.
struct Detection {
    QString id;
    qint64 timestampMs = 0;  // epoch ms UTC — format only at paint time
    Severity severity = Severity::Info;
    QString ruleId;
    QString ruleName;
    QString processName;
    QString techniqueId;

    QString ruleDescription;
    QString detectionReason;
    QString recommendedAction;
    int confidence = 0;    // 0–100
    int occurrences = 1;
    QString host;
    QString user;
    quint32 pid = 0;
    quint32 parentPid = 0;
    QString parentProcessName;
    QString commandLine;
    QVector<Ioc> iocs;
    QVector<QString> relatedIds;
    QString rawEventJson;
};

struct CorrelationAlert {
    QString id;
    QString title;
    QString description;
    Severity severity = Severity::Info;
    int confidence = 0;  // 0–100
    qint64 timestampMs = 0;
    int matchedEventCount = 0;
    QVector<QString> mitreTechniques;
};

struct LogLine {
    qint64 timestampMs = 0;
    LogLevel level = LogLevel::Info;
    QString component;
    QString message;
};

struct CollectorStats {
    quint64 eventsProcessed = 0;
    quint64 detections = 0;
    quint64 correlationAlerts = 0;
    int rulesLoaded = 0;
    int sigmaRulesLoaded = 0;
    bool correlationEnabled = true;
    double cpuPercent = 0.0;
    double memoryMb = 0.0;
    double eventsPerSecond = 0.0;
    double pipelineLatencyMs = 0.0;
};

// Static reference data from the source (not live telemetry).
struct RuleInfo {
    QString ruleId;
    QString name;
    QString description;
    QString author;
    QString version;
    QString sigmaSource;
    Severity severity = Severity::Info;
    QString techniqueId;
    QString logicSummary;
    QString knownFalsePositives;
    QString investigationNotes;
};

inline QString severityLabel(Severity severity) {
    switch (severity) {
        case Severity::Critical:
            return QStringLiteral("CRITICAL");
        case Severity::High:
            return QStringLiteral("HIGH");
        case Severity::Medium:
            return QStringLiteral("MEDIUM");
        case Severity::Low:
            return QStringLiteral("LOW");
        case Severity::Info:
        default:
            return QStringLiteral("INFO");
    }
}

inline QString connectionStateLabel(ConnectionState state) {
    switch (state) {
        case ConnectionState::Connected:
            return QStringLiteral("Connected");
        case ConnectionState::Connecting:
            return QStringLiteral("Connecting");
        case ConnectionState::Reconnecting:
            return QStringLiteral("Reconnecting");
        case ConnectionState::Failed:
            return QStringLiteral("Failed");
        case ConnectionState::Disconnected:
        default:
            return QStringLiteral("Disconnected");
    }
}

inline QString logLevelLabel(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return QStringLiteral("TRACE");
        case LogLevel::Debug:
            return QStringLiteral("DEBUG");
        case LogLevel::Info:
            return QStringLiteral("INFO");
        case LogLevel::Warn:
            return QStringLiteral("WARN");
        case LogLevel::Error:
            return QStringLiteral("ERROR");
        case LogLevel::Fatal:
            return QStringLiteral("FATAL");
        default:
            return QStringLiteral("INFO");
    }
}

inline QString iocTypeLabel(IocType type) {
    switch (type) {
        case IocType::Process:
            return QStringLiteral("Process");
        case IocType::User:
            return QStringLiteral("User");
        case IocType::Host:
            return QStringLiteral("Host");
        case IocType::Ip:
            return QStringLiteral("IP");
        case IocType::Domain:
            return QStringLiteral("Domain");
        case IocType::Url:
            return QStringLiteral("URL");
        case IocType::RegistryKey:
            return QStringLiteral("Registry");
        case IocType::Hash:
            return QStringLiteral("Hash");
        case IocType::Service:
            return QStringLiteral("Service");
        case IocType::ScheduledTask:
            return QStringLiteral("Scheduled task");
        default:
            return QStringLiteral("IOC");
    }
}

}  // namespace sentinelforge

Q_DECLARE_METATYPE(sentinelforge::Severity)
Q_DECLARE_METATYPE(sentinelforge::ConnectionState)
Q_DECLARE_METATYPE(sentinelforge::LogLevel)
Q_DECLARE_METATYPE(sentinelforge::IocType)
Q_DECLARE_METATYPE(sentinelforge::Ioc)
Q_DECLARE_METATYPE(sentinelforge::Detection)
Q_DECLARE_METATYPE(sentinelforge::CorrelationAlert)
Q_DECLARE_METATYPE(sentinelforge::LogLine)
Q_DECLARE_METATYPE(sentinelforge::CollectorStats)
Q_DECLARE_METATYPE(sentinelforge::RuleInfo)
Q_DECLARE_METATYPE(QVector<sentinelforge::Detection>)
Q_DECLARE_METATYPE(QVector<sentinelforge::CorrelationAlert>)
Q_DECLARE_METATYPE(QVector<sentinelforge::LogLine>)
Q_DECLARE_METATYPE(QVector<sentinelforge::Ioc>)
