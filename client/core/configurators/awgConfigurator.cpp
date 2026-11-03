#include "awgConfigurator.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"

#include <QDebug>
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

    if (container == DockerContainer::Awg2) {
        QString startupOutput;
        auto cbReadStartup = [&](const QString &data, libssh::Client &) {
            startupOutput += data + "\n";
            return ErrorCode::NoError;
        };

        const QString containerName = ContainerUtils::containerToString(container);
        const QString startupCheck = QString(
                "sudo docker exec -i %1 sh -c 'for i in 1 2 3 4 5 6 7 8 9 10; do "
                "if awg show awg0 >/dev/null 2>&1; then echo AMNEZIA_AWG_READY; exit 0; fi; "
                "sleep 1; done; echo AMNEZIA_AWG_STARTUP_FAILED; "
                "cat /opt/amnezia/awg/startup.log 2>/dev/null || true'")
                                             .arg(containerName);

        const ErrorCode startupError =
                m_sshSession->runScript(credentials, startupCheck, cbReadStartup, cbReadStartup);
        if (startupError != ErrorCode::NoError || !startupOutput.contains("AMNEZIA_AWG_READY")) {
            qWarning().noquote() << "AmneziaWG interface failed to start:" << startupOutput;
            errorCode = startupError != ErrorCode::NoError ? startupError : ErrorCode::ServerCheckFailed;
            return AwgProtocolConfig{};
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

    if (container == DockerContainer::Awg2) {
        QString peersOutput;
        auto cbReadPeers = [&](const QString &data, libssh::Client &) {
            peersOutput += data + "\n";
            return ErrorCode::NoError;
        };

        const QString peerCheck = QString("sudo docker exec -i %1 awg show awg0 peers")
                                          .arg(ContainerUtils::containerToString(container));
        const ErrorCode peerCheckError =
                m_sshSession->runScript(credentials, peerCheck, cbReadPeers, cbReadPeers);
        if (peerCheckError != ErrorCode::NoError
            || !peersOutput.contains(wgConfig->clientConfig->clientPublicKey)) {
            qWarning().noquote() << "AmneziaWG peer was not applied:" << peersOutput;
            errorCode = peerCheckError != ErrorCode::NoError ? peerCheckError : ErrorCode::ServerCheckFailed;
            return AwgProtocolConfig{};
        }
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
    
    newClientConfig.cookieReplyPacketJunkSize = configMap.value(configKey::cookieReplyPacketJunkSize);
    newClientConfig.transportPacketJunkSize = configMap.value(configKey::transportPacketJunkSize);

    newClientConfig.headerProtectionKey = configMap.value(configKey::headerProtectionKey);
    newClientConfig.contentPaddingAddition = configMap.value(configKey::contentPaddingAddition);
    newClientConfig.rekeyAfterTime = configMap.value(configKey::rekeyAfterTime);
    newClientConfig.rekeyTimeout = configMap.value(configKey::rekeyTimeout);
    newClientConfig.rejectAfterTime = configMap.value(configKey::rejectAfterTime);
    newClientConfig.keepaliveTimeout = configMap.value(configKey::keepaliveTimeout);
    newClientConfig.maxHandshakeAttempts = configMap.value(configKey::maxHandshakeAttempts);
    newClientConfig.randomTrailers = configMap.value(configKey::randomTrailers);
    newClientConfig.disableCookies = configMap.value(configKey::disableCookies);

    protocolConfig.setClientConfig(newClientConfig);
    
    return protocolConfig;
}
