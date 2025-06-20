#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QJsonObject>
#include <QSharedPointer>
#include <QString>

#include "core/defs.h"
#include "core/models/containers/containerConfig.h"

class ServerConfig
{
public:
    ServerConfig(const QJsonObject &serverConfigObject);

    virtual QJsonObject toJson() const;

    static QSharedPointer<ServerConfig> createServerConfig(const QJsonObject &serverConfigObject);

    amnezia::ServerConfigType type;

    QString hostName;

    QString dns1;
    QString dns2;

    QString defaultContainer;

    QMap<QString, ContainerConfig> containerConfigs;
};

#endif // SERVERCONFIG_H
