#include "xray_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/scripts_registry.h"

XrayConfigurator::XrayConfigurator(std::shared_ptr<Settings> settings, QObject *parent) : ConfiguratorBase(settings, parent)
{
}

QString XrayConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig,
                                       ErrorCode errorCode)
{
    ServerController serverController(m_settings);

    QString config = serverController.replaceVars(amnezia::scriptData(ProtocolScriptType::xray_template, container),
                                                  serverController.genVarsForScript(credentials, container, containerConfig));

    QString xrayPublicKey =
            serverController.getTextFileFromContainer(container, credentials, amnezia::protocols::xray::PublicKeyPath, errorCode);
    xrayPublicKey.replace("\n", "");

    QString xrayUuid = serverController.getTextFileFromContainer(container, credentials, amnezia::protocols::xray::uuidPath, errorCode);
    xrayUuid.replace("\n", "");

    QString xrayShortId = serverController.getTextFileFromContainer(container, credentials, amnezia::protocols::xray::shortidPath, errorCode);
    xrayShortId.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    config.replace("$XRAY_CLIENT_ID", xrayUuid);
    config.replace("$XRAY_PUBLIC_KEY", xrayPublicKey);
    config.replace("$XRAY_SHORT_ID", xrayShortId);

    return config;
}
