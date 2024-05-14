#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                                 QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

QString ShadowSocksConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                              const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    QString ssKey =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::shadowsocks::ssKeyPath, errorCode);
    ssKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
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

    // qDebug().noquote() << textCfg;
    return textCfg;
}
