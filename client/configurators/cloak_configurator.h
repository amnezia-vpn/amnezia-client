#ifndef CLOAK_CONFIGURATOR_H
#define CLOAK_CONFIGURATOR_H

#include <QObject>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class CloakConfigurator
{
public:

    static QString genCloakConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);
};

#endif // CLOAK_CONFIGURATOR_H
