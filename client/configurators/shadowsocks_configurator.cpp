#include "shadowsocks_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "containers/containers_defs.h"
#include "core/scripts_registry.h"
#include "core/servercontroller.h"

ShadowSocksConfigurator::ShadowSocksConfigurator(std::shared_ptr<Settings> settings,
                                                 QObject *parent): ConfiguratorBase(settings, parent)
{
}

QString ShadowSocksConfigurator::genShadowSocksConfig(const ServerCredentials &credentials, DockerContainer container,
                                                      const QJsonObject &containerConfig, ErrorCode *errorCode)
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

    QString ssClientConfig = serverController.replaceVars(amnezia::scriptData(ProtocolScriptType::shadowsocks_client_template, container),
                                                          serverController.genVarsForScript(credentials, container, containerConfig));

    ssClientConfig.replace("$SHADOWSOCKS_PASSWORD", ssKey);
    ssClientConfig = serverController.replaceVars(ssClientConfig, serverController.genVarsForScript(credentials, container, containerConfig));

    //qDebug().noquote() << textCfg;
    return ssClientConfig;
}
