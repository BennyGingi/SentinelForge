#include "models/DetectionFilterProxy.h"

#include <QRegularExpression>

#include "models/DetectionModel.h"

namespace sentinelforge {

DetectionFilterState parseSearchQuery(const QString& query) {
    DetectionFilterState state;
    const QStringList parts = query.split(QRegularExpression(QStringLiteral("\\s+")),
                                          Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const int colon = part.indexOf(QLatin1Char(':'));
        if (colon > 0) {
            SearchToken token;
            token.field = part.left(colon).toLower();
            token.value = part.mid(colon + 1).toLower();
            if (token.field == QLatin1String("severity") || token.field == QLatin1String("sev")) {
                if (token.value.startsWith(QLatin1String("crit"))) {
                    state.severity = Severity::Critical;
                } else if (token.value.startsWith(QLatin1String("high"))) {
                    state.severity = Severity::High;
                } else if (token.value.startsWith(QLatin1String("med"))) {
                    state.severity = Severity::Medium;
                } else if (token.value.startsWith(QLatin1String("low"))) {
                    state.severity = Severity::Low;
                } else if (token.value.startsWith(QLatin1String("info"))) {
                    state.severity = Severity::Info;
                }
            } else if (token.field == QLatin1String("host") ||
                       token.field == QLatin1String("hostname")) {
                state.host = token.value;
            } else if (token.field == QLatin1String("process") ||
                       token.field == QLatin1String("proc")) {
                state.process = token.value;
            } else if (token.field == QLatin1String("rule")) {
                state.rule = token.value;
            } else if (token.field == QLatin1String("technique") ||
                       token.field == QLatin1String("mitre") ||
                       token.field == QLatin1String("t")) {
                state.technique = token.value;
            } else if (token.field == QLatin1String("user") ||
                       token.field == QLatin1String("username")) {
                token.field = QStringLiteral("user");
                state.tokens.push_back(token);
            } else {
                state.tokens.push_back(token);
            }
        } else {
            SearchToken token;
            token.value = part.toLower();
            state.tokens.push_back(token);
        }
    }
    return state;
}

DetectionFilterProxy::DetectionFilterProxy(QObject* parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    setSortRole(Qt::DisplayRole);
}

void DetectionFilterProxy::setFilterState(DetectionFilterState state) {
    state_ = std::move(state);
    invalidateFilter();
}

void DetectionFilterProxy::setRawQuery(const QString& query) {
    rawQuery_ = query;
    DetectionFilterState parsed = parseSearchQuery(query);
    // Preserve combo-driven fields that aren't in the query text.
    parsed.minTimestampMs = state_.minTimestampMs;
    if (!state_.severity.has_value()) {
        // keep parsed severity from query
    } else if (!parsed.severity.has_value()) {
        parsed.severity = state_.severity;
    }
    // Host/process/rule/technique from combos take precedence if set separately —
    // merge: explicit state fields from UI overlays.
    setFilterState(std::move(parsed));
}

bool DetectionFilterProxy::matchesTokens(const QModelIndex& srcIndex) const {
    const QString blob = srcIndex.data(DetectionModel::SearchBlobRole).toString();
    const Detection det = srcIndex.data(DetectionModel::DetectionRole).value<Detection>();

    if (state_.severity.has_value() && det.severity != *state_.severity) {
        return false;
    }
    if (state_.minTimestampMs > 0 && det.timestampMs < state_.minTimestampMs) {
        return false;
    }
    if (!state_.host.isEmpty() && !det.host.toLower().contains(state_.host)) {
        return false;
    }
    if (!state_.process.isEmpty() && !det.processName.toLower().contains(state_.process)) {
        return false;
    }
    if (!state_.rule.isEmpty() && !det.ruleName.toLower().contains(state_.rule)) {
        return false;
    }
    if (!state_.technique.isEmpty() && !det.techniqueId.toLower().contains(state_.technique)) {
        return false;
    }

    for (const SearchToken& token : state_.tokens) {
        if (token.field.isEmpty()) {
            if (!blob.contains(token.value)) {
                return false;
            }
            continue;
        }
        if (token.field == QLatin1String("user")) {
            if (!det.user.toLower().contains(token.value)) {
                return false;
            }
        } else if (token.field == QLatin1String("cmdline") ||
                   token.field == QLatin1String("command")) {
            if (!det.commandLine.toLower().contains(token.value)) {
                return false;
            }
        } else if (token.field == QLatin1String("pid")) {
            if (!QString::number(det.pid).contains(token.value)) {
                return false;
            }
        } else if (token.field == QLatin1String("ioc")) {
            bool hit = false;
            for (const Ioc& ioc : det.iocs) {
                if (ioc.value.toLower().contains(token.value)) {
                    hit = true;
                    break;
                }
            }
            if (!hit) {
                return false;
            }
        } else if (token.field == QLatin1String("service") ||
                   token.field == QLatin1String("registry") ||
                   token.field == QLatin1String("registrykey")) {
            bool hit = false;
            for (const Ioc& ioc : det.iocs) {
                if ((token.field.startsWith(QLatin1String("reg")) &&
                     ioc.type == IocType::RegistryKey) ||
                    (token.field == QLatin1String("service") && ioc.type == IocType::Service)) {
                    if (ioc.value.toLower().contains(token.value)) {
                        hit = true;
                        break;
                    }
                }
            }
            if (!hit && !blob.contains(token.value)) {
                return false;
            }
            if (!hit) {
                return false;
            }
        } else {
            if (!blob.contains(token.value)) {
                return false;
            }
        }
    }
    return true;
}

bool DetectionFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    return matchesTokens(idx);
}

bool DetectionFilterProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    const int col = left.column();
    if (col == DetectionModel::Timestamp) {
        return left.data(DetectionModel::TimestampMsRole).toLongLong() <
               right.data(DetectionModel::TimestampMsRole).toLongLong();
    }
    if (col == DetectionModel::SeverityCol) {
        return left.data(DetectionModel::SeverityRole).value<Severity>() <
               right.data(DetectionModel::SeverityRole).value<Severity>();
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

}  // namespace sentinelforge
