#include "awg_configurator.h"
#include "protocols/protocols_defs.h"

AwgConfigurator::AwgConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : WireguardConfigurator(settings, serverController, true, parent)
{
}

QSharedPointer<ProtocolConfig> AwgConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                          const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode)
{
    auto result = WireguardConfigurator::createConfig(credentials, container, protocolConfig, errorCode);
    if (!result) {
        return nullptr;
    }
    
    auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(result);
    if (!awgConfig) {
        errorCode = ErrorCode::InternalError;
        return nullptr;
    }
    
    QString config = awgConfig->clientProtocolConfig.nativeConfig;

    QMap<QString, QString> configMap;
    auto configLines = config.split("\n");
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

    awgConfig->clientProtocolConfig.wireGuardData.mtu = awgConfig->serverProtocolConfig.mtu;

    return awgConfig;
}
