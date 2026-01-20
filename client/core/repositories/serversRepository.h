#ifndef SERVERSREPOSITORY_H
#define SERVERSREPOSITORY_H

#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"

using namespace amnezia;

class ServersRepository
{
public:
    virtual ~ServersRepository() = default;

    virtual void addServer(const ServerConfig &server) = 0;
    virtual void editServer(int index, const ServerConfig &server) = 0;
    virtual void removeServer(int index) = 0;
    virtual ServerConfig server(int index) const = 0;
    virtual QVector<ServerConfig> servers() const = 0;
    virtual int serversCount() const = 0;

    virtual int defaultServerIndex() const = 0;
    virtual void setDefaultServer(int index) = 0;

    virtual void setDefaultContainer(int serverIndex, DockerContainer container) = 0;
    virtual ContainerConfig containerConfig(int serverIndex, DockerContainer container) const = 0;
    virtual void setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config) = 0;
    virtual void clearLastConnectionConfig(int serverIndex, DockerContainer container) = 0;

    // Utilities
    virtual ServerCredentials serverCredentials(int index) const = 0;
    virtual bool hasServerWithVpnKey(const QString &vpnKey) const = 0;
};

#endif // SERVERSREPOSITORY_H

