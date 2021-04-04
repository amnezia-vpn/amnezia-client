#ifndef CLOAK_CONFIGURATOR_H
#define CLOAK_CONFIGURATOR_H

#include <QObject>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class CloakConfigurator
{
public:

    static QJsonObject genCloakConfig(const ServerCredentials &credentials, Protocol proto,
        ErrorCode *errorCode = nullptr);
};

#endif // CLOAK_CONFIGURATOR_H
