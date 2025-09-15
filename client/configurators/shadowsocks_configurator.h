#ifndef SHADOWSOCKS_CONFIGURATOR_H
#define SHADOWSOCKS_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/defs.h"

class ShadowSocksConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    ShadowSocksConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                            QObject *parent = nullptr);

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                ErrorCode &errorCode) override;
};

#endif // SHADOWSOCKS_CONFIGURATOR_H
