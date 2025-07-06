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

    jsonConfig[config_key::junkPacketCount] = configMap.value(config_key::junkPacketCount);
    jsonConfig[config_key::junkPacketMinSize] = configMap.value(config_key::junkPacketMinSize);
    jsonConfig[config_key::junkPacketMaxSize] = configMap.value(config_key::junkPacketMaxSize);
    jsonConfig[config_key::initPacketJunkSize] = configMap.value(config_key::initPacketJunkSize);
    jsonConfig[config_key::responsePacketJunkSize] = configMap.value(config_key::responsePacketJunkSize);
    jsonConfig[config_key::initPacketMagicHeader] = configMap.value(config_key::initPacketMagicHeader);
    jsonConfig[config_key::responsePacketMagicHeader] = configMap.value(config_key::responsePacketMagicHeader);
    jsonConfig[config_key::underloadPacketMagicHeader] = configMap.value(config_key::underloadPacketMagicHeader);
    jsonConfig[config_key::transportPacketMagicHeader] = configMap.value(config_key::transportPacketMagicHeader);

    // jsonConfig[config_key::cookieReplyPacketJunkSize] = configMap.value(config_key::cookieReplyPacketJunkSize);
    // jsonConfig[config_key::transportPacketJunkSize] = configMap.value(config_key::transportPacketJunkSize);

    // jsonConfig[config_key::specialJunk1] = configMap.value(amnezia::config_key::specialJunk1);
    // jsonConfig[config_key::specialJunk2] = configMap.value(amnezia::config_key::specialJunk2);
    // jsonConfig[config_key::specialJunk3] = configMap.value(amnezia::config_key::specialJunk3);
    // jsonConfig[config_key::specialJunk4] = configMap.value(amnezia::config_key::specialJunk4);
    // jsonConfig[config_key::specialJunk5] = configMap.value(amnezia::config_key::specialJunk5);
    // jsonConfig[config_key::controlledJunk1] = configMap.value(amnezia::config_key::controlledJunk1);
    // jsonConfig[config_key::controlledJunk2] = configMap.value(amnezia::config_key::controlledJunk2);
    // jsonConfig[config_key::controlledJunk3] = configMap.value(amnezia::config_key::controlledJunk3);
    // jsonConfig[config_key::specialHandshakeTimeout] = configMap.value(amnezia::config_key::specialHandshakeTimeout);

    jsonConfig[config_key::mtu] =
            containerConfig.value(ProtocolProps::protoToString(Proto::Awg)).toObject().value(config_key::mtu).toString(protocols::awg::defaultMtu);

    return QJsonDocument(jsonConfig).toJson();
}
