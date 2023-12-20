#include "cloak_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/controllers/serverController.h"
#include "containers/containers_defs.h"

CloakConfigurator::CloakConfigurator(std::shared_ptr<Settings> settings, QObject *parent):
    ConfiguratorBase(settings, parent)
{

}

QString CloakConfigurator::genCloakConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;
    ServerController serverController(m_settings);

    QString cloakPublicKey = serverController.getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckPublicKeyPath, &e);
    cloakPublicKey.replace("\n", "");

    QString cloakBypassUid = serverController.getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckBypassUidKeyPath, &e);
    cloakBypassUid.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
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

    QString textCfg = serverController.replaceVars(QJsonDocument(config).toJson(),
                                                   serverController.genVarsForScript(credentials, container, containerConfig));

    // qDebug().noquote() << textCfg;
    return textCfg;
}
