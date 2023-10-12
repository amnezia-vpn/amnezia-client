#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings, QObject *parent):
    ConfiguratorBase(settings, parent)
{

}

QString ShadowSocksConfigurator::genShadowSocksConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;
    ServerController serverController(m_settings);

    QString ssKey = serverController.getTextFileFromContainer(container, credentials,
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


    QString textCfg = serverController.replaceVars(QJsonDocument(config).toJson(),
                                                   serverController.genVarsForScript(credentials, container, containerConfig));

    //qDebug().noquote() << textCfg;
    return textCfg;
}
