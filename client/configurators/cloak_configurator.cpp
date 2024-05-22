#include "cloak_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"

CloakConfigurator::CloakConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

QString CloakConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                                        ErrorCode &errorCode)
{
    QString cloakPublicKey =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::cloak::ckPublicKeyPath, errorCode);
    cloakPublicKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    QString cloakBypassUid =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::cloak::ckBypassUidKeyPath, errorCode);
    cloakBypassUid.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return "";
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
    config.insert("RemoteHost", credentials.hostName);
    config.insert("RemotePort", "$CLOAK_SERVER_PORT");

    QString textCfg = m_serverController->replaceVars(QJsonDocument(config).toJson(),
                                                      m_serverController->genVarsForScript(credentials, container, containerConfig));

    return textCfg;
}
