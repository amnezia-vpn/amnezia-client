#include "v2ray_configurator.h"

#include <QFile>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/servercontroller.h"
#include "core/scripts_registry.h"
#include "containers/containers_defs.h"

V2RayConfigurator::V2RayConfigurator(std::shared_ptr<Settings> settings,
                                     std::shared_ptr<ServerController> serverController,
                                     QObject *parent) : ConfiguratorBase(settings, serverController, parent)
{
}

QString V2RayConfigurator::genV2RayConfig(const ServerCredentials &credentials, DockerContainer container,
                                          const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    QString v2RayVmessClientUuid = m_serverController->getTextFileFromContainer(container, credentials,
                                                                                amnezia::protocols::v2ray::v2rayKeyPath, &e);
    if (v2RayVmessClientUuid.isEmpty()) {
        e = ErrorCode::V2RayKeyMissing;
        return "";
    }

    v2RayVmessClientUuid.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return "";
    }

    QString v2RayClientConfig = m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::v2ray_client_template, container),
                                                                m_serverController->genVarsForScript(credentials, container, containerConfig));

    v2RayClientConfig.replace("$V2RAY_VMESS_CLIENT_UUID", v2RayVmessClientUuid);
    v2RayClientConfig = m_serverController->replaceVars(v2RayClientConfig,
                                                        m_serverController->genVarsForScript(credentials, container, containerConfig));

    return v2RayClientConfig;
}
