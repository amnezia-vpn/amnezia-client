#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "core/servercontroller.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings, std::shared_ptr<ServerController> serverController, QObject *parent):
    ConfiguratorBase(settings, serverController, parent)
{

}

QString ShadowSocksConfigurator::genShadowSocksConfig(const ServerCredentials &credentials, DockerContainer container,
                                                      const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    QString ssKey = m_serverController->getTextFileFromContainer(container, credentials,
        amnezia::protocols::shadowsocks::ssKeyPath, errorCode);
    ssKey.replace("\n", "");

    if (errorCode) {
        return "";
    }

    QJsonObject config;
    config.insert("server", credentials.hostName);
    config.insert("server_port", "$SHADOWSOCKS_SERVER_PORT");
    config.insert("local_port", "$SHADOWSOCKS_LOCAL_PORT");
    config.insert("password", ssKey);
    config.insert("timeout", 60);
    config.insert("method", "$SHADOWSOCKS_CIPHER");


    QString textCfg = m_serverController->replaceVars(QJsonDocument(config).toJson(),
        m_serverController->genVarsForScript(credentials, container, containerConfig));

    //qDebug().noquote() << textCfg;
    return textCfg;
}
