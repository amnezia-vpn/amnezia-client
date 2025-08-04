#ifndef CLOAK_CONFIGURATOR_H
#define CLOAK_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/models/protocols/cloakProtocolConfig.h"

using namespace amnezia;

class CloakConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    CloakConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) override;

    Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                             const QSharedPointer<ProtocolConfig> &protocolConfig) const override;
};

#endif // CLOAK_CONFIGURATOR_H
