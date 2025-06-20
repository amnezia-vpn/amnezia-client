#ifndef CONTAINERCONFIG_H
#define CONTAINERCONFIG_H

#include <QMap>
#include <QSharedPointer>
#include <QString>

#include "core/models/protocols/protocolConfig.h"

class ContainerConfig
{
public:
    ContainerConfig();

    QString containerName;
    QMap<QString, QSharedPointer<ProtocolConfig>> protocolConfigs;
};

#endif // CONTAINERCONFIG_H
