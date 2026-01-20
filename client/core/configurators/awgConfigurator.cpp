#include "awgConfigurator.h"
#include "core/protocols/protocolsDefs.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"

#include <QJsonDocument>
#include <QJsonObject>

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
    
    if (auto* awgConfigModel = std::get_if<AwgProtocolConfig>(&containerConfig.protocolConfig)) {
        serverConfig = &awgConfigModel->serverConfig;
        if (awgConfigModel->clientConfig.has_value()) {
            clientConfig = &awgConfigModel->clientConfig.value();
        }
    }
    
    ProtocolConfig wireguardConfig = WireguardConfigurator::createConfig(credentials, container, containerConfig, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return AwgProtocolConfig{};
    }
    
    WireGuardProtocolConfig* wgConfig = std::get_if<WireGuardProtocolConfig>(&wireguardConfig);
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
    
    newClientConfig.junkPacketCount = configMap.value(config_key::junkPacketCount);
    newClientConfig.junkPacketMinSize = configMap.value(config_key::junkPacketMinSize);
    newClientConfig.junkPacketMaxSize = configMap.value(config_key::junkPacketMaxSize);
    newClientConfig.initPacketJunkSize = configMap.value(config_key::initPacketJunkSize);
    newClientConfig.responsePacketJunkSize = configMap.value(config_key::responsePacketJunkSize);
    newClientConfig.initPacketMagicHeader = configMap.value(config_key::initPacketMagicHeader);
    newClientConfig.responsePacketMagicHeader = configMap.value(config_key::responsePacketMagicHeader);
    newClientConfig.underloadPacketMagicHeader = configMap.value(config_key::underloadPacketMagicHeader);
    newClientConfig.transportPacketMagicHeader = configMap.value(config_key::transportPacketMagicHeader);
    newClientConfig.specialJunk1 = configMap.value(amnezia::config_key::specialJunk1);
    newClientConfig.specialJunk2 = configMap.value(amnezia::config_key::specialJunk2);
    newClientConfig.specialJunk3 = configMap.value(amnezia::config_key::specialJunk3);
    newClientConfig.specialJunk4 = configMap.value(amnezia::config_key::specialJunk4);
    newClientConfig.specialJunk5 = configMap.value(amnezia::config_key::specialJunk5);
    
    if (container == DockerContainer::Awg2) {
        newClientConfig.cookieReplyPacketJunkSize = configMap.value(config_key::cookieReplyPacketJunkSize);
        newClientConfig.transportPacketJunkSize = configMap.value(config_key::transportPacketJunkSize);
    }
    
    newClientConfig.isObfuscationEnabled = false;
    
    protocolConfig.setClientConfig(newClientConfig);
    
    return protocolConfig;
}
