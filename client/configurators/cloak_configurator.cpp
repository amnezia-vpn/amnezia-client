#include "cloak_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/servercontroller.h"
#include "containers/containers_defs.h"

CloakConfigurator::CloakConfigurator(std::shared_ptr<Settings> settings, std::shared_ptr<ServerController> serverController, QObject *parent):
    ConfiguratorBase(settings, serverController, parent)
{

}

QString CloakConfigurator::genCloakConfig(const ServerCredentials &credentials, DockerContainer container,
                                          const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    QString cloakPublicKey = m_serverController->getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckPublicKeyPath, errorCode);
    cloakPublicKey.replace("\n", "");

    QString cloakBypassUid = m_serverController->getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckBypassUidKeyPath, errorCode);
    cloakBypassUid.replace("\n", "");

    if (errorCode) {
        return "";
    }

    QJsonObject config;
    config.insert("Transport", "direct");
    config.insert("ProxyMethod", "openvpn");
    config.insert("EncryptionMethod", "aes-gcm");
    config.insert("UID", cloakBypassUid);
    config.insert("PublicKey", cloakPublicKey);
    config.insert("ServerName", "$FAKE_WEB_SITE_ADDRESS");
    config.insert("NumConn", 4);
    config.insert("BrowserSig", "chrome");
    config.insert("StreamTimeout", 300);

    // transfer params to protocol runner
    config.insert(config_key::transport_proto, "tcp");
    config.insert(config_key::remote, credentials.hostName);
    config.insert(config_key::port, "$CLOAK_SERVER_PORT");

    QString textCfg = m_serverController->replaceVars(QJsonDocument(config).toJson(),
        m_serverController->genVarsForScript(credentials, container, containerConfig));

    // qDebug().noquote() << textCfg;
    return textCfg;
}
