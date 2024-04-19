#ifndef CLOAK_CONFIGURATOR_H
#define CLOAK_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"

using namespace amnezia;

class CloakConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    CloakConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    QString createConfig(const ServerCredentials &credentials, DockerContainer container,
                         const QJsonObject &containerConfig, ErrorCode errorCode);
};

#endif // CLOAK_CONFIGURATOR_H
