#include "mitre/MitreCatalog.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

namespace sentinelforge {

MitreCatalog& MitreCatalog::instance() {
    static MitreCatalog catalog;
    return catalog;
}

bool MitreCatalog::load() {
    if (loaded_) {
        return true;
    }
    QFile file(QStringLiteral(":/mitre/techniques.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }
    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QJsonObject o = it.value().toObject();
        MitreTechnique t;
        t.id = it.key();
        t.name = o.value(QStringLiteral("name")).toString();
        t.subtechnique = o.value(QStringLiteral("subtechnique")).toString();
        t.description = o.value(QStringLiteral("description")).toString();
        byId_.insert(t.id, t);
    }
    loaded_ = true;
    return true;
}

MitreTechnique MitreCatalog::lookup(const QString& techniqueId) const {
    return byId_.value(techniqueId);
}

bool MitreCatalog::isValidTechniqueId(const QString& techniqueId) {
    static const QRegularExpression re(QStringLiteral("^T\\d{4}(\\.\\d{3})?$"));
    return re.match(techniqueId).hasMatch();
}

QUrl MitreCatalog::attackUrl(const QString& techniqueId) {
    if (!isValidTechniqueId(techniqueId)) {
        return {};
    }
    // Build URL only from the validated ID — never from raw unvalidated field text.
    const QString path = techniqueId.contains(QLatin1Char('.'))
                             ? QStringLiteral("techniques/%1/%2")
                                   .arg(techniqueId.section(QLatin1Char('.'), 0, 0),
                                        techniqueId.section(QLatin1Char('.'), 1, 1))
                             : QStringLiteral("techniques/%1").arg(techniqueId);
    return QUrl(QStringLiteral("https://attack.mitre.org/%1/").arg(path));
}

}  // namespace sentinelforge
