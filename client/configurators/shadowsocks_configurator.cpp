#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

QString ShadowSocksConfigurator::genShadowSocksConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    QString ssKey = ServerController::getTextFileFromContainer(container, credentials,
        amnezia::protocols::shadowsocks::ssKeyPath, &e);
    ssKey.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return "";
    }

    QJsonObject config;
    config.insert("server", credentials.hostName);
    config.insert("server_port", "$SHADOWSOCKS_SERVER_PORT");
    config.insert("local_port", "$SHADOWSOCKS_LOCAL_PORT");
    config.insert("password", ssKey);
    config.insert("timeout", 60);
    config.insert("method", "$SHADOWSOCKS_CIPHER");


    QString textCfg = ServerController::replaceVars(QJsonDocument(config).toJson(),
        ServerController::genVarsForScript(credentials, container, containerConfig));

    //qDebug().noquote() << textCfg;
    return textCfg;
}
