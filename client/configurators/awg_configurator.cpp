#include "awg_configurator.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/servercontroller.h"

AwgConfigurator::AwgConfigurator(std::shared_ptr<Settings> settings, QObject *parent)
    : WireguardConfigurator(settings, true, parent)
{
}

QString AwgConfigurator::genAwgConfig(const ServerCredentials &credentials,
                                                                DockerContainer container,
                                                                const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QString config = WireguardConfigurator::genWireguardConfig(credentials, container, containerConfig, errorCode);

    ServerController serverController(m_settings);
    QString serverConfig = serverController.getTextFileFromContainer(container, credentials, protocols::awg::serverConfigPath, errorCode);

    QMap<QString, QString> serverConfigMap;
    auto serverConfigLines = serverConfig.split("\n");
    for (auto &line : serverConfigLines) {
        auto trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                serverConfigMap.insert(parts[0].trimmed(), parts[1].trimmed());
            }
        }
    }

    config.replace("$JUNK_PACKET_COUNT", serverConfigMap.value(config_key::junkPacketCount));
    config.replace("$JUNK_PACKET_MIN_SIZE", serverConfigMap.value(config_key::junkPacketMinSize));
    config.replace("$JUNK_PACKET_MAX_SIZE", serverConfigMap.value(config_key::junkPacketMaxSize));
    config.replace("$INIT_PACKET_JUNK_SIZE", serverConfigMap.value(config_key::initPacketJunkSize));
    config.replace("$RESPONSE_PACKET_JUNK_SIZE", serverConfigMap.value(config_key::responsePacketJunkSize));
    config.replace("$INIT_PACKET_MAGIC_HEADER", serverConfigMap.value(config_key::initPacketMagicHeader));
    config.replace("$RESPONSE_PACKET_MAGIC_HEADER", serverConfigMap.value(config_key::responsePacketMagicHeader));
    config.replace("$UNDERLOAD_PACKET_MAGIC_HEADER", serverConfigMap.value(config_key::underloadPacketMagicHeader));
    config.replace("$TRANSPORT_PACKET_MAGIC_HEADER", serverConfigMap.value(config_key::transportPacketMagicHeader));

    QJsonObject jsonConfig = QJsonDocument::fromJson(config.toUtf8()).object();

    jsonConfig[config_key::junkPacketCount] = serverConfigMap.value(config_key::junkPacketCount);
    jsonConfig[config_key::junkPacketMinSize] = serverConfigMap.value(config_key::junkPacketMinSize);
    jsonConfig[config_key::junkPacketMaxSize] = serverConfigMap.value(config_key::junkPacketMaxSize);
    jsonConfig[config_key::initPacketJunkSize] = serverConfigMap.value(config_key::initPacketJunkSize);
    jsonConfig[config_key::responsePacketJunkSize] = serverConfigMap.value(config_key::responsePacketJunkSize);
    jsonConfig[config_key::initPacketMagicHeader] = serverConfigMap.value(config_key::initPacketMagicHeader);
    jsonConfig[config_key::responsePacketMagicHeader] = serverConfigMap.value(config_key::responsePacketMagicHeader);
    jsonConfig[config_key::underloadPacketMagicHeader] = serverConfigMap.value(config_key::underloadPacketMagicHeader);
    jsonConfig[config_key::transportPacketMagicHeader] = serverConfigMap.value(config_key::transportPacketMagicHeader);

    return QJsonDocument(jsonConfig).toJson();
}
