#ifndef VPN_CONFIGURATOR_H
#define VPN_CONFIGURATOR_H

#include <QObject>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class VpnConfigurator
{
public:

    static QString genVpnProtocolConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, Protocol proto, ErrorCode *errorCode = nullptr);
};

#endif // VPN_CONFIGURATOR_H
