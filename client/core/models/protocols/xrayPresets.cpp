#include "xrayPresets.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUuid>

#include "logger.h"

#include "core/models/protocols/xrayInbound.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/utils/constants/protocolConstants.h"

namespace
{
    Logger logger("XrayPresets");

    namespace px = amnezia::protocols::xray;

    struct BuiltinDefinition
    {
        const char *id;
        const char *name;
        const char *description;
        const char *transport;
        const char *security;
        const char *flow;
        bool padding;
        bool recommended;
    };

    constexpr BuiltinDefinition builtinDefinitions[] = {
        { amnezia::xrayPresets::defaultPresetId, QT_TRANSLATE_NOOP("XrayPresets", "REALITY · RAW · Vision"),
          QT_TRANSLATE_NOOP("XrayPresets", "Fastest. Looks like traffic to a regular website"), px::transportRaw,
          px::securityReality, px::defaultFlow, false, true },
        { "reality-xhttp", QT_TRANSLATE_NOOP("XrayPresets", "REALITY · XHTTP"),
          QT_TRANSLATE_NOOP("XrayPresets", "Looks like regular HTTP traffic, works behind a CDN"), px::transportXhttp,
          px::securityReality, "", false, false },
        { "tls-raw-vision", QT_TRANSLATE_NOOP("XrayPresets", "TLS · RAW · Vision"),
          QT_TRANSLATE_NOOP("XrayPresets", "Needs your own domain and certificate"), px::transportRaw, px::securityTls,
          px::defaultFlow, false, false },
        { "tls-xhttp-padding", QT_TRANSLATE_NOOP("XrayPresets", "TLS · XHTTP with padding"),
          QT_TRANSLATE_NOOP("XrayPresets", "XHTTP with padding in cookies"), px::transportXhttp, px::securityTls, "",
          true, false },
        { "tls-mkcp", QT_TRANSLATE_NOOP("XrayPresets", "TLS · mKCP"),
          QT_TRANSLATE_NOOP("XrayPresets", "UDP transport for networks with packet loss"), px::transportMkcp,
          px::securityTls, "", false, false },
    };

    QString translate(const char *text)
    {
        return QCoreApplication::translate("XrayPresets", text);
    }

    QString builtinInbound(const BuiltinDefinition &definition, const QString &site, const QString &port)
    {
        amnezia::XrayServerConfig srv;
        srv.port = port.isEmpty() ? QString::fromLatin1(px::defaultPort) : port;
        srv.transport = QString::fromLatin1(definition.transport);
        srv.security = QString::fromLatin1(definition.security);
        srv.flow = QString::fromLatin1(definition.flow);
        srv.site = site;
        srv.sni = site;
        srv.alpn = QString::fromLatin1(px::defaultAlpn);
        srv.fingerprint = QString::fromLatin1(px::defaultFingerprint);
        srv.xhttp.xPadding.obfsMode = definition.padding;
        srv.xhttp.applyRangeDefaults();
        return amnezia::xrayInbound::templateFromServerConfig(srv);
    }
}

namespace amnezia::xrayPresets
{

    QList<XrayPreset> builtins(const QString &site, const QString &port)
    {
        QList<XrayPreset> result;
        for (const BuiltinDefinition &definition : builtinDefinitions) {
            XrayPreset preset;
            preset.id = QString::fromLatin1(definition.id);
            preset.name = translate(definition.name);
            preset.description = translate(definition.description);
            preset.inbound = builtinInbound(definition, site, port);
            preset.builtin = true;
            preset.recommended = definition.recommended;
            result.append(preset);
        }
        logger.info() << "builtins: generated" << QString::number(result.size()) << "for site" << site << "port" << port;
        return result;
    }

    QString matchBuiltin(const QString &inbound, const QString &site, const QString &port)
    {
        const QString target = xrayInbound::normalized(inbound);
        for (const XrayPreset &preset : builtins(site, port)) {
            if (xrayInbound::normalized(preset.inbound) == target) {
                logger.info() << "matchBuiltin: matched" << preset.id;
                return preset.id;
            }
        }
        logger.info() << "matchBuiltin: no built-in preset matches";
        return {};
    }

    QList<XrayPreset> customFromStorage(const QByteArray &data)
    {
        QList<XrayPreset> result;
        const QJsonArray array = QJsonDocument::fromJson(data).array();
        for (const QJsonValue &value : array) {
            const QJsonObject object = value.toObject();
            XrayPreset preset;
            preset.id = object.value(QStringLiteral("id")).toString();
            preset.name = object.value(QStringLiteral("name")).toString();
            preset.inbound = object.value(QStringLiteral("inbound")).toString();
            preset.createdAt = QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
            if (preset.id.isEmpty() || preset.inbound.isEmpty()) {
                logger.warning() << "customFromStorage: skipped broken entry" << preset.id;
                continue;
            }
            result.append(preset);
        }
        logger.info() << "customFromStorage: loaded" << QString::number(result.size()) << "bytes"
                      << QString::number(data.size());
        return result;
    }

    QByteArray customToStorage(const QList<XrayPreset> &presets)
    {
        QJsonArray array;
        for (const XrayPreset &preset : presets) {
            array.append(QJsonObject { { QStringLiteral("id"), preset.id },
                                       { QStringLiteral("name"), preset.name },
                                       { QStringLiteral("createdAt"), preset.createdAt.toString(Qt::ISODate) },
                                       { QStringLiteral("inbound"), preset.inbound } });
        }
        const QByteArray data = QJsonDocument(array).toJson(QJsonDocument::Compact);
        logger.info() << "customToStorage: saved" << QString::number(presets.size()) << "bytes"
                      << QString::number(data.size());
        return data;
    }

    QList<XrayPreset> customFromLegacySnapshots(const QByteArray &snapshots)
    {
        QList<XrayPreset> result;
        const QJsonArray array = QJsonDocument::fromJson(snapshots).array();
        for (const QJsonValue &value : array) {
            const QJsonObject object = value.toObject();
            XrayServerConfig srv = XrayServerConfig::fromJson(object.value(QStringLiteral("serverConfig")).toObject());
            XrayPreset preset;
            preset.id = object.value(QStringLiteral("id")).toString();
            if (preset.id.isEmpty()) {
                preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            }
            preset.name = object.value(QStringLiteral("displayName")).toString();
            preset.createdAt = QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
            preset.inbound = xrayInbound::templateFromServerConfig(srv);
            result.append(preset);
            logger.info() << "customFromLegacySnapshots: migrated" << preset.id << preset.name;
        }
        ensureUniqueIds(result);
        logger.info() << "customFromLegacySnapshots: migrated" << QString::number(result.size()) << "of"
                      << QString::number(array.size());
        return result;
    }

    bool ensureUniqueIds(QList<XrayPreset> &presets)
    {
        QSet<QString> seen;
        bool changed = false;
        for (XrayPreset &preset : presets) {
            if (preset.id.isEmpty() || seen.contains(preset.id)) {
                const QString previous = preset.id;
                preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                changed = true;
                logger.warning() << "ensureUniqueIds: duplicate id" << previous << "replaced with" << preset.id << "for"
                                 << preset.name;
            }
            seen.insert(preset.id);
        }
        return changed;
    }

    XrayPreset makeCustom(const QString &name, const QString &inbound)
    {
        XrayPreset preset;
        preset.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        preset.name = name.trimmed();
        preset.inbound = inbound;
        preset.createdAt = QDateTime::currentDateTime();
        logger.info() << "makeCustom:" << preset.id << preset.name << "length" << QString::number(inbound.size());
        return preset;
    }

} // namespace amnezia::xrayPresets
