#include "v2ray_configurator.h"

V2RayConfigurator::V2RayConfigurator(std::shared_ptr<Settings> settings, std::shared_ptr<ServerController> serverController,
                                     QObject *parent) : ConfiguratorBase(settings, serverController, parent)
{

}

QString V2RayConfigurator::genCloakConfig(const ServerCredentials &credentials, DockerContainer container,
                                          const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    return "";
}
