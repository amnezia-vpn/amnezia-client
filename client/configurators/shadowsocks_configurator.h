#ifndef SHADOWSOCKS_CONFIGURATOR_H
#define SHADOWSOCKS_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/defs.h"

class ShadowSocksConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    ShadowSocksConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) override;

    Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                             const QSharedPointer<ProtocolConfig> &protocolConfig) const override;
};

#endif // SHADOWSOCKS_CONFIGURATOR_H
