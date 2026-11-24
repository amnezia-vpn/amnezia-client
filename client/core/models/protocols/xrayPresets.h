#ifndef XRAYPRESETS_H
#define XRAYPRESETS_H

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

namespace amnezia
{

    struct XrayPreset
    {
        QString id;
        QString name;
        QString description;
        QString inbound;
        QDateTime createdAt;
        bool builtin = false;
        bool recommended = false;
    };

    namespace xrayPresets
    {
        inline constexpr char defaultPresetId[] = "reality-raw-vision";

        QList<XrayPreset> builtins(const QString &site, const QString &port);
        QString matchBuiltin(const QString &inbound, const QString &site, const QString &port);

        QList<XrayPreset> customFromStorage(const QByteArray &data);
        QByteArray customToStorage(const QList<XrayPreset> &presets);
        QList<XrayPreset> customFromLegacySnapshots(const QByteArray &snapshots);
        bool ensureUniqueIds(QList<XrayPreset> &presets);

        XrayPreset makeCustom(const QString &name, const QString &inbound);
    }

} // namespace amnezia

#endif // XRAYPRESETS_H
