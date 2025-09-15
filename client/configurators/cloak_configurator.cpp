#include "cloak_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/models/protocols/cloakProtocolConfig.h"

CloakConfigurator::CloakConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                     QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

QSharedPointer<ProtocolConfig> CloakConfigurator::createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                               ErrorCode &errorCode)
{
    QString cloakPublicKey = m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials,
                                                                          amnezia::protocols::cloak::ckPublicKeyPath, errorCode);
    cloakPublicKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    QString cloakBypassUid = m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials,
                                                                          amnezia::protocols::cloak::ckBypassUidKeyPath, errorCode);
    cloakBypassUid.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    QJsonObject config;
    config.insert("Transport", "direct");
    config.insert("ProxyMethod", "openvpn");
    config.insert("EncryptionMethod", "aes-gcm");
    config.insert("UID", cloakBypassUid);
    config.insert("PublicKey", cloakPublicKey);
    config.insert("ServerName", "$FAKE_WEB_SITE_ADDRESS");
    config.insert("NumConn", 1);
    config.insert("BrowserSig", "chrome");
    config.insert("StreamTimeout", 300);
    config.insert("RemoteHost", serverCredentials.hostName);
    config.insert("RemotePort", "$CLOAK_SERVER_PORT");

    QString textCfg = m_serverController->replaceVars(QJsonDocument(config).toJson(),
                                                      m_serverController->genVarsForScript(containerConfig, serverCredentials));

    auto cloakConfig = QSharedPointer<CloakProtocolConfig>::create(QJsonObject(), config_key::cloak);
    cloakConfig->clientProtocolConfig.isEmpty = false;
    cloakConfig->clientProtocolConfig.nativeConfig = textCfg;

    return cloakConfig;
}
