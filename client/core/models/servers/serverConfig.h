#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QJsonObject>
#include <QSharedPointer>
#include <QString>

#include "core/defs.h"
#include "core/models/containers/containerConfig.h"

class SelfHostedServerConfig;
class ApiV1ServerConfig;
class ApiV2ServerConfig;

using ServerConfigVariant =
        std::variant<QSharedPointer<SelfHostedServerConfig>, QSharedPointer<ApiV1ServerConfig>, QSharedPointer<ApiV2ServerConfig> >;

class ServerConfig
{
public:
    ServerConfig(const QJsonObject &serverConfigObject);

    virtual QJsonObject toJson() const;

    static QSharedPointer<ServerConfig> createServerConfig(const QJsonObject &serverConfigObject);
    static ServerConfigVariant getServerConfigVariant(const QSharedPointer<ServerConfig> &serverConfig);

    void updateProtocolConfig(const QString &containerName, const QMap<QString, QSharedPointer<ProtocolConfig>> &protocolConfigs);

    amnezia::ServerConfigType type;

    QString hostName;

    QString dns1;
    QString dns2;

    QString defaultContainer;

    bool nameOverriddenByUser;
    int crc; // TODO it makes sense to add for all server types or move it to the api

    QMap<QString, ContainerConfig> containerConfigs;
};

#endif // SERVERCONFIG_H
