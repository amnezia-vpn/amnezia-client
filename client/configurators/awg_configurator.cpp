#include "awg_configurator.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "core/servercontroller.h"

AwgConfigurator::AwgConfigurator(std::shared_ptr<Settings> settings, QObject *parent)
    : WireguardConfigurator(settings, true, parent)
{
}

QString AwgConfigurator::genAwgConfig(const ServerCredentials &credentials, DockerContainer container,
                                      const QJsonObject &containerConfig, QString &clientId, ErrorCode *errorCode)
{
    QString config = WireguardConfigurator::genWireguardConfig(credentials, container, containerConfig, clientId, errorCode);

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

    return QJsonDocument(jsonConfig).toJson();
}
