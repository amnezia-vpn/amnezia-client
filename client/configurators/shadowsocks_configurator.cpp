#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "core/scripts_registry.h"
#include "core/servercontroller.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings,
                                                 std::shared_ptr<ServerController> serverController,
                                                 QObject *parent): ConfiguratorBase(settings, serverController, parent)
{
}

QString ShadowSocksConfigurator::genShadowSocksConfig(const ServerCredentials &credentials, DockerContainer container,
                                                      const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    QString ssKey = m_serverController->getTextFileFromContainer(container, credentials,
                                                                 amnezia::protocols::shadowsocks::ssKeyPath, &e);
    ssKey.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return "";
    }

    QString ssClientConfig = m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::shadowsocks_client_template, container),
                                                             m_serverController->genVarsForScript(credentials, container, containerConfig));

    ssClientConfig.replace("$SHADOWSOCKS_PASSWORD", ssKey);
    ssClientConfig = m_serverController->replaceVars(ssClientConfig, m_serverController->genVarsForScript(credentials, container, containerConfig));

    //qDebug().noquote() << textCfg;
    return ssClientConfig;
}
