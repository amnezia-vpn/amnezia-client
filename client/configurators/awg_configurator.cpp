#include "awg_configurator.h"
#include "protocols/protocols_defs.h"

#include <QJsonDocument>
#include <QJsonObject>

AwgConfigurator::AwgConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : WireguardConfigurator(settings, serverController, true, parent)
{
}

QString AwgConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                                      ErrorCode &errorCode)
{
    QString config = WireguardConfigurator::createConfig(credentials, container, containerConfig, errorCode);

    QJsonObject jsonConfig = QJsonDocument::fromJson(config.toUtf8()).object();
    QString awgConfig = jsonConfig.value(config_key::config).toString();

    QMap<QString, QString> configMap;
    auto configLines = awgConfig.split("\n");
    for (auto &line : configLines) {
        auto trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                configMap.insert(parts[0].trimmed(), parts[1].trimmed());
            }
        }
    }

    auto insertIfNotEmpty = [&](const QString &key) {
        const QString value = configMap.value(key).trimmed();
        if (!value.isEmpty()) {
            jsonConfig[key] = value;
        } else {
            jsonConfig.remove(key);
        }
    };

    insertIfNotEmpty(config_key::junkPacketCount);
    insertIfNotEmpty(config_key::junkPacketMinSize);
    insertIfNotEmpty(config_key::junkPacketMaxSize);
    insertIfNotEmpty(config_key::initPacketJunkSize);
    insertIfNotEmpty(config_key::responsePacketJunkSize);
    insertIfNotEmpty(config_key::initPacketMagicHeader);
    insertIfNotEmpty(config_key::responsePacketMagicHeader);
    insertIfNotEmpty(config_key::underloadPacketMagicHeader);
    insertIfNotEmpty(config_key::transportPacketMagicHeader);

    if (container == DockerContainer::Awg2) {
        insertIfNotEmpty(config_key::cookieReplyPacketJunkSize);
        insertIfNotEmpty(config_key::transportPacketJunkSize);
        jsonConfig[config_key::protocolVersion] = protocols::awg::awgV2;
    }

    insertIfNotEmpty(amnezia::config_key::specialJunk1);
    insertIfNotEmpty(amnezia::config_key::specialJunk2);
    insertIfNotEmpty(amnezia::config_key::specialJunk3);
    insertIfNotEmpty(amnezia::config_key::specialJunk4);
    insertIfNotEmpty(amnezia::config_key::specialJunk5);

    const QString allowedIpsValue = configMap.value(QStringLiteral("AllowedIPs"));
    if (!allowedIpsValue.isEmpty()) {
        QJsonArray allowedIps;
        const auto parts = allowedIpsValue.split(',', Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            const QString trimmed = part.trimmed();
            if (!trimmed.isEmpty()) {
                allowedIps.append(trimmed);
            }
        }
        if (!allowedIps.isEmpty()) {
            jsonConfig[config_key::allowed_ips] = allowedIps;
        }
    }

    jsonConfig[config_key::mtu] =
            containerConfig.value(ProtocolProps::protoToString(Proto::Awg)).toObject().value(config_key::mtu).toString(protocols::awg::defaultMtu);

    return QJsonDocument(jsonConfig).toJson();
}
