#include "cloak_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

QString CloakConfigurator::genCloakConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    QString cloakPublicKey = ServerController::getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckPublicKeyPath, &e);
    cloakPublicKey.replace("\n", "");

    QString cloakBypassUid = ServerController::getTextFileFromContainer(container, credentials,
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
    config.insert("NumConn", 4);
    config.insert("BrowserSig", "chrome");
    config.insert("StreamTimeout", 300);

    // transfer params to protocol runner
    config.insert(config_key::transport_proto, "$OPENVPN_TRANSPORT_PROTO");
    config.insert(config_key::remote, credentials.hostName);
    config.insert(config_key::port, "$CLOAK_SERVER_PORT");

    QString textCfg = ServerController::replaceVars(QJsonDocument(config).toJson(),
        ServerController::genVarsForScript(credentials, container, containerConfig));

    // qDebug().noquote() << textCfg;
    return textCfg;
}
