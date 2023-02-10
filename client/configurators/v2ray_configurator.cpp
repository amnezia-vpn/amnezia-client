#include "v2ray_configurator.h"

#include <QFile>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "core/servercontroller.h"
#include "containers/containers_defs.h"

V2RayConfigurator::V2RayConfigurator(std::shared_ptr<Settings> settings, std::shared_ptr<ServerController> serverController,
                                     QObject *parent) : ConfiguratorBase(settings, serverController, parent)
{

}

QString V2RayConfigurator::genV2RayConfig(const ServerCredentials &credentials, DockerContainer container,
                                          const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ErrorCode e = ErrorCode::NoError;

    QString v2rayVmessClientUuid = m_serverController->getTextFileFromContainer(container, credentials,
                                                                                amnezia::protocols::v2ray::v2rayKeyPath, &e);
    v2rayVmessClientUuid.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return "";
    }

    QJsonObject config;

    QJsonObject inboundsSocks;
    inboundsSocks.insert("protocol", "socks");
    inboundsSocks.insert("listen", "127.0.0.1");
    inboundsSocks.insert("port", 1080); //todo
    QJsonObject socksSettings;
    socksSettings.insert("auth", "noauth");
    socksSettings.insert("udp", true);
    inboundsSocks.insert("settings", socksSettings);

    QJsonArray inbounds;
    inbounds.push_back(inboundsSocks);
    config.insert("inbounds", inbounds);


    QJsonObject outboundsVmess;
    outboundsVmess.insert("protocol", "vmess");
    QJsonObject vmessSettings;
    QJsonObject vnext;
    vnext.insert("address", credentials.hostName);
    vnext.insert("port", 10086); //todo
    QJsonObject users;
    users.insert("id", v2rayVmessClientUuid);
    vnext.insert("users", QJsonArray({users}));
    vmessSettings.insert("vnext", QJsonArray({vnext}));
    outboundsVmess.insert("settings", vmessSettings);

    QJsonArray outbounds;
    outbounds.push_back(outboundsVmess);
    config.insert("outbounds", outbounds);


    QString textCfg = m_serverController->replaceVars(QJsonDocument(config).toJson(),
                                                      m_serverController->genVarsForScript(credentials, container, containerConfig));

         // qDebug().noquote() << textCfg;
    return textCfg;
}
