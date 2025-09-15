#ifndef SERVERCONFIGCONTROLLER_H
#define SERVERCONFIGCONTROLLER_H

#include "core/models/servers/serverConfig.h"
#include "settings.h"

class ServerConfigController
{
public:
    ServerConfigController();

    // void addServer(const QSharedPointer<ServerConfig> &serverConfig);
    // void editServer(const QSharedPointer<ServerConfig> &serverConfig, const int serverIndex);
    // void removeServer(const int serverIndex);

    // QSharedPointer<ServerConfig> getServer(const int serverIndex);
    // QVector<QSharedPointer<ServerConfig>> getAllServers();

    // void setDefaultServerIndex(const int index);
    // const int getDefaultServerIndex();

private:
    std::shared_ptr<Settings> m_settings;
};

#endif // SERVERCONFIGCONTROLLER_H
