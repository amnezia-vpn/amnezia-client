#include "cloak_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

QJsonObject CloakConfigurator::genCloakConfig(const ServerCredentials &credentials,
    Protocol proto, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    DockerContainer container = amnezia::containerForProto(proto);

    QString cloakPublicKey = ServerController::getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckPublicKeyPath(), &e);
    cloakPublicKey.replace("\n", "");

    QString cloakBypassUid = ServerController::getTextFileFromContainer(container, credentials,
        amnezia::protocols::cloak::ckBypassUidKeyPath(), &e);
    cloakBypassUid.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return QJsonObject();
    }

    QJsonObject config;
    config.insert("Transport", "direct");
    config.insert("ProxyMethod", "openvpn");
    config.insert("EncryptionMethod", "aes-gcm");
    config.insert("UID", cloakBypassUid);
    config.insert("PublicKey", cloakPublicKey);
    config.insert("ServerName", amnezia::protocols::cloak::ckDefaultRedirSite());
    config.insert("NumConn", 4);
    config.insert("BrowserSig", "chrome");
    config.insert("StreamTimeout", 300);

    // Amnezia field
    config.insert("Remote", credentials.hostName);

    qDebug().noquote() << QJsonDocument(config).toJson();
    return config;
}
