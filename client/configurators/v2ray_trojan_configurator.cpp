#include "v2ray_trojan_configurator.h"

#include <QFile>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/servercontroller.h"
#include "core/scripts_registry.h"
#include "containers/containers_defs.h"

V2RayTrojanConfigurator::V2RayTrojanConfigurator(std::shared_ptr<Settings> settings,
                                                 std::shared_ptr<ServerController> serverController,
                                                 QObject *parent) : ConfiguratorBase(settings, serverController, parent)
{
}

QString V2RayTrojanConfigurator::genV2RayConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    QString clientPassword = m_serverController->getTextFileFromContainer(container, credentials,
                                                                          amnezia::protocols::v2ray_trojan::v2rayTrojanPasswordPath, &e);
    if (clientPassword.isEmpty()) {
        *errorCode = ErrorCode::V2RayTrojanPasswordMissing;
        return "";
    }

    QString v2RayClientConfig = m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::v2ray_trojan_client_template, container),
                                                                m_serverController->genVarsForScript(credentials, container, containerConfig));

    v2RayClientConfig.replace("$V2RAY_TROJAN_CLIENT_PASSWORD", clientPassword);
    v2RayClientConfig = m_serverController->replaceVars(v2RayClientConfig,
                                                        m_serverController->genVarsForScript(credentials, container, containerConfig));

    return v2RayClientConfig;
}
