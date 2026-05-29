#include "awgConfigurator.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonObject>

using namespace amnezia;

AwgConfigurator::AwgConfigurator(SshSession* sshSession, QObject *parent)
    : WireguardConfigurator(sshSession, true, parent)
{
}

ProtocolConfig AwgConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &containerConfig,
                                              const DnsSettings &dnsSettings,
                                              ErrorCode &errorCode)
{
    const AwgServerConfig* serverConfig = nullptr;
    const AwgClientConfig* clientConfig = nullptr;
    
    if (auto* awgProtocolConfig = containerConfig.getAwgProtocolConfig()) {
        serverConfig = &awgProtocolConfig->serverConfig;
        if (awgProtocolConfig->clientConfig.has_value()) {
            clientConfig = &awgProtocolConfig->clientConfig.value();
        }
    }
    
    ProtocolConfig wireguardConfig = WireguardConfigurator::createConfig(credentials, container, containerConfig, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return AwgProtocolConfig{};
    }
    
    WireGuardProtocolConfig* wgConfig = wireguardConfig.as<WireGuardProtocolConfig>();
    if (!wgConfig || !wgConfig->clientConfig.has_value()) {
        errorCode = ErrorCode::InternalError;
        return AwgProtocolConfig{};
    }
    
    QString awgConfig = wgConfig->clientConfig->nativeConfig;

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

    AwgProtocolConfig protocolConfig;
    if (serverConfig) {
        protocolConfig.serverConfig = *serverConfig;
    }
    
    AwgClientConfig newClientConfig;
    newClientConfig.nativeConfig = awgConfig;
    newClientConfig.hostName = wgConfig->clientConfig->hostName;
    newClientConfig.port = wgConfig->clientConfig->port;
    newClientConfig.clientIp = wgConfig->clientConfig->clientIp;
    newClientConfig.clientPrivateKey = wgConfig->clientConfig->clientPrivateKey;
    newClientConfig.clientPublicKey = wgConfig->clientConfig->clientPublicKey;
    newClientConfig.serverPublicKey = wgConfig->clientConfig->serverPublicKey;
    newClientConfig.presharedKey = wgConfig->clientConfig->presharedKey;
    newClientConfig.clientId = wgConfig->clientConfig->clientId;
    newClientConfig.allowedIps = wgConfig->clientConfig->allowedIps;
    newClientConfig.persistentKeepAlive = wgConfig->clientConfig->persistentKeepAlive;
    
    QString mtu = protocols::awg::defaultMtu;
    if (clientConfig && !clientConfig->mtu.isEmpty()) {
        mtu = clientConfig->mtu;
    }
    newClientConfig.mtu = mtu;
    
    newClientConfig.junkPacketCount = configMap.value(configKey::junkPacketCount);
    newClientConfig.junkPacketMinSize = configMap.value(configKey::junkPacketMinSize);
    newClientConfig.junkPacketMaxSize = configMap.value(configKey::junkPacketMaxSize);
    newClientConfig.initPacketJunkSize = configMap.value(configKey::initPacketJunkSize);
    newClientConfig.responsePacketJunkSize = configMap.value(configKey::responsePacketJunkSize);
    newClientConfig.initPacketMagicHeader = configMap.value(configKey::initPacketMagicHeader);
    newClientConfig.responsePacketMagicHeader = configMap.value(configKey::responsePacketMagicHeader);
    newClientConfig.underloadPacketMagicHeader = configMap.value(configKey::underloadPacketMagicHeader);
    newClientConfig.transportPacketMagicHeader = configMap.value(configKey::transportPacketMagicHeader);
    newClientConfig.specialJunk1 = configMap.value(configKey::specialJunk1);
    newClientConfig.specialJunk2 = configMap.value(configKey::specialJunk2);
    newClientConfig.specialJunk3 = configMap.value(configKey::specialJunk3);
    newClientConfig.specialJunk4 = configMap.value(configKey::specialJunk4);
    newClientConfig.specialJunk5 = configMap.value(configKey::specialJunk5);
    
    if (container == DockerContainer::Awg2) {
        newClientConfig.cookieReplyPacketJunkSize = configMap.value(configKey::cookieReplyPacketJunkSize);
        newClientConfig.transportPacketJunkSize = configMap.value(configKey::transportPacketJunkSize);
    }
    
    newClientConfig.isObfuscationEnabled = false;
    
    protocolConfig.setClientConfig(newClientConfig);
    
    return protocolConfig;
}
