#ifndef SERVERSREPOSITORY_H
#define SERVERSREPOSITORY_H

#include <QJsonObject>
#include <QJsonArray>

#include "containers/containers_defs.h"
#include "core/defs.h"

using namespace amnezia;

class ServersRepository
{
public:
    virtual ~ServersRepository() = default;

    // Server CRUD operations
    virtual void addServer(const QJsonObject &server) = 0;
    virtual void editServer(int index, const QJsonObject &server) = 0;
    virtual void removeServer(int index) = 0;
    virtual QJsonObject server(int index) const = 0;
    virtual QJsonArray serversArray() const = 0;
    virtual int serversCount() const = 0;

    // Default server management
    virtual int defaultServerIndex() const = 0;
    virtual void setDefaultServer(int index) = 0;

    // Container management
    virtual void setDefaultContainer(int serverIndex, DockerContainer container) = 0;
    virtual QJsonObject containerConfig(int serverIndex, DockerContainer container) const = 0;
    virtual void setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config) = 0;
    virtual void clearLastConnectionConfig(int serverIndex, DockerContainer container) = 0;

    // Utilities
    virtual ServerCredentials serverCredentials(int index) const = 0;
    virtual bool hasServerWithVpnKey(const QString &vpnKey) const = 0;
};

#endif // SERVERSREPOSITORY_H

