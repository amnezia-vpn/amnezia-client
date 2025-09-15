#ifndef CONTAINERCONFIG_H
#define CONTAINERCONFIG_H

#include <QMap>
#include <QString>

#include "core/models/protocols/protocolConfig.h"
#include "containers/containers_defs.h"

class ContainerConfig
{
public:
    ContainerConfig();

    bool isClientProtocolConfigEmpty();

    QString containerName;
    DockerContainer containerType;

    QMap<QString, QSharedPointer<ProtocolConfig>> protocolConfigs;
};

#endif // CONTAINERCONFIG_H
