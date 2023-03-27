#ifndef V2RAYTROJANCONFIGURATOR_H
#define V2RAYTROJANCONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"

using namespace amnezia;

class V2RayTrojanConfigurator : ConfiguratorBase
{
public:
    V2RayTrojanConfigurator(std::shared_ptr<Settings> settings,
                            std::shared_ptr<ServerController> serverController,
                            QObject *parent = nullptr);

    QString genV2RayConfig(const ServerCredentials &credentials, DockerContainer container,
                           const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);
};

#endif // V2RAYTROJANCONFIGURATOR_H

