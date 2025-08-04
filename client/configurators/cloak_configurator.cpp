#include "cloak_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/models/containers/containers_defs.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "protocols/protocols_defs.h"

CloakConfigurator::CloakConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

ConfiguratorBase::Vars CloakConfigurator::generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                                                               const QSharedPointer<ProtocolConfig> &protocolConfig) const
{
    Vars vars = generateCommonVars(credentials, container);

    auto cloakConfig = qSharedPointerCast<CloakProtocolConfig>(protocolConfig);
    if (!cloakConfig) {
        return vars;
    }

    vars.append({{"$CLOAK_SERVER_PORT", cloakConfig->serverProtocolConfig.port}});
    vars.append({{"$FAKE_WEB_SITE_ADDRESS", cloakConfig->serverProtocolConfig.site}});

    return vars;
}

QSharedPointer<ProtocolConfig> CloakConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                            const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode)
{
    auto cloakConfig = qSharedPointerCast<CloakProtocolConfig>(protocolConfig);
    if (!cloakConfig) {
        errorCode = ErrorCode::InternalError;
        return nullptr;
    }

    QString cloakPublicKey =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::cloak::ckPublicKeyPath, errorCode);
    cloakPublicKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    QString cloakBypassUid =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::cloak::ckBypassUidKeyPath, errorCode);
    cloakBypassUid.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    cloakConfig->clientProtocolConfig.transport = "direct";
    cloakConfig->clientProtocolConfig.proxyMethod = "openvpn";
    cloakConfig->clientProtocolConfig.encryptionMethod = "aes-gcm";
    cloakConfig->clientProtocolConfig.uid = cloakBypassUid;
    cloakConfig->clientProtocolConfig.publicKey = cloakPublicKey;
    cloakConfig->clientProtocolConfig.serverName = cloakConfig->serverProtocolConfig.site;
    cloakConfig->clientProtocolConfig.numConn = 1;
    cloakConfig->clientProtocolConfig.browserSig = "chrome";
    cloakConfig->clientProtocolConfig.streamTimeout = 300;
    cloakConfig->clientProtocolConfig.remoteHost = credentials.hostName;
    cloakConfig->clientProtocolConfig.remotePort = cloakConfig->serverProtocolConfig.port;

    QJsonObject config;
    config.insert("Transport", cloakConfig->clientProtocolConfig.transport);
    config.insert("ProxyMethod", cloakConfig->clientProtocolConfig.proxyMethod);
    config.insert("EncryptionMethod", cloakConfig->clientProtocolConfig.encryptionMethod);
    config.insert("UID", cloakConfig->clientProtocolConfig.uid);
    config.insert("PublicKey", cloakConfig->clientProtocolConfig.publicKey);
    config.insert("ServerName", cloakConfig->clientProtocolConfig.serverName);
    config.insert("NumConn", cloakConfig->clientProtocolConfig.numConn);
    config.insert("BrowserSig", cloakConfig->clientProtocolConfig.browserSig);
    config.insert("StreamTimeout", cloakConfig->clientProtocolConfig.streamTimeout);
    config.insert("RemoteHost", cloakConfig->clientProtocolConfig.remoteHost);
    config.insert("RemotePort", cloakConfig->clientProtocolConfig.remotePort);

    QString textCfg = QJsonDocument(config).toJson();

    cloakConfig->clientProtocolConfig.isEmpty = false;
    cloakConfig->clientProtocolConfig.nativeConfig = textCfg;

    return cloakConfig;
}
