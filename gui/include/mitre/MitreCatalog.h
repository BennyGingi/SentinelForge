#pragma once

#include <QHash>
#include <QString>
#include <QUrl>

namespace sentinelforge {

struct MitreTechnique {
    QString id;
    QString name;
    QString subtechnique;
    QString description;
};

// Presentation reference data only — not detection logic.
// Loaded once from :/mitre/techniques.json.
class MitreCatalog final {
public:
    static MitreCatalog& instance();

    bool load();
    MitreTechnique lookup(const QString& techniqueId) const;
    static bool isValidTechniqueId(const QString& techniqueId);
    static QUrl attackUrl(const QString& techniqueId);

private:
    MitreCatalog() = default;
    QHash<QString, MitreTechnique> byId_;
    bool loaded_ = false;
};

}  // namespace sentinelforge
