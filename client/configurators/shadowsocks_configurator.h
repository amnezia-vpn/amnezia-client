#ifndef SHADOWSOCKS_CONFIGURATOR_H
#define SHADOWSOCKS_CONFIGURATOR_H

#include <QObject>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class ShadowSocksConfigurator
{
public:

    static QString genShadowSocksConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);
};

#endif // SHADOWSOCKS_CONFIGURATOR_H
