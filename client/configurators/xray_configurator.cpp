#include "xray_configurator.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/scripts_registry.h"
#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"

XrayConfigurator::XrayConfigurator(std::shared_ptr<Settings> settings, QObject *parent):
    ConfiguratorBase(settings, parent)
{

}

QString XrayConfigurator::genXrayConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, QString &clientId, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;
    ServerController serverController(m_settings);

    QString config =
        serverController.replaceVars(amnezia::scriptData(ProtocolScriptType::xray_template, container),
                                     serverController.genVarsForScript(credentials, container, containerConfig));

    QString xrayPublicKey = serverController.getTextFileFromContainer(container, credentials,
                                                                       amnezia::protocols::xray::PublicKeyPath, &e);
    xrayPublicKey.replace("\n", "");

    QString xrayUuid = serverController.getTextFileFromContainer(container, credentials,
                                                                       amnezia::protocols::xray::uuidPath, &e);
    xrayUuid.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return "";
    }

    config.replace("$XRAY_CLIENT_ID", xrayUuid);
    config.replace("$XRAY_PUBLIC_KEY", xrayPublicKey);

    return config;
}
