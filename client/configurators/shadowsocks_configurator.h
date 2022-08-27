#ifndef SHADOWSOCKS_CONFIGURATOR_H
#define SHADOWSOCKS_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/defs.h"

class ShadowSocksConfigurator : ConfiguratorBase
{
    Q_OBJECT
public:
    ShadowSocksConfigurator(std::shared_ptr<Settings> settings,
        std::shared_ptr<ServerController> serverController, QObject *parent = nullptr);

    QString genShadowSocksConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);
};

#endif // SHADOWSOCKS_CONFIGURATOR_H
