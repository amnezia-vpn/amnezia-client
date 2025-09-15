#include "awg_configurator.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "protocols/protocols_defs.h"

#include <QJsonDocument>
#include <QJsonObject>

AwgConfigurator::AwgConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : WireguardConfigurator(settings, serverController, true, parent)
{
}

QSharedPointer<ProtocolConfig> AwgConfigurator::createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                             ErrorCode &errorCode)
{
    auto baseConfig = WireguardConfigurator::createConfig(serverCredentials, containerConfig, errorCode);
    if (!baseConfig || errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(baseConfig);
    if (!awgConfig) {
        return nullptr;
    }

    QString nativeConfig = awgConfig->clientProtocolConfig.nativeConfig;

    QMap<QString, QString> configMap;
    auto configLines = nativeConfig.split("\n");
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

    awgConfig->clientProtocolConfig.awgData.junkPacketCount = configMap.value(config_key::junkPacketCount);
    awgConfig->clientProtocolConfig.awgData.junkPacketMinSize = configMap.value(config_key::junkPacketMinSize);
    awgConfig->clientProtocolConfig.awgData.junkPacketMaxSize = configMap.value(config_key::junkPacketMaxSize);
    awgConfig->clientProtocolConfig.awgData.initPacketJunkSize = configMap.value(config_key::initPacketJunkSize);
    awgConfig->clientProtocolConfig.awgData.responsePacketJunkSize = configMap.value(config_key::responsePacketJunkSize);
    awgConfig->clientProtocolConfig.awgData.initPacketMagicHeader = configMap.value(config_key::initPacketMagicHeader);
    awgConfig->clientProtocolConfig.awgData.responsePacketMagicHeader = configMap.value(config_key::responsePacketMagicHeader);
    awgConfig->clientProtocolConfig.awgData.underloadPacketMagicHeader = configMap.value(config_key::underloadPacketMagicHeader);
    awgConfig->clientProtocolConfig.awgData.transportPacketMagicHeader = configMap.value(config_key::transportPacketMagicHeader);

    awgConfig->serverProtocolConfig.awgData = awgConfig->clientProtocolConfig.awgData;

    awgConfig->clientProtocolConfig.wireGuardData.mtu = protocols::awg::defaultMtu;

    if (containerConfig.protocolConfigs.contains(config_key::awg)) {
        if (auto existingAwgConfig = qSharedPointerCast<AwgProtocolConfig>(containerConfig.protocolConfigs.value(config_key::awg))) {
            if (!existingAwgConfig->clientProtocolConfig.wireGuardData.mtu.isEmpty()) {
                awgConfig->clientProtocolConfig.wireGuardData.mtu = existingAwgConfig->clientProtocolConfig.wireGuardData.mtu;
            }
        }
    }

    return awgConfig;
}
