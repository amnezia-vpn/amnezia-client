#ifndef INSTALLERBASE_H
#define INSTALLERBASE_H

#include <QObject>
#include <QJsonObject>

#include "core/utils/containerEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/containerConfig.h"

class InstallerBase : public QObject
{
    Q_OBJECT
public:
    explicit InstallerBase(QObject *parent = nullptr);

    virtual amnezia::ContainerConfig generateConfig(amnezia::DockerContainer container, int port, amnezia::TransportProto transportProto);

    virtual amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                                 SshSession* sshSession, amnezia::ContainerConfig &config);

    amnezia::ContainerConfig createBaseConfig(amnezia::DockerContainer container, int port, amnezia::TransportProto transportProto);

protected:
};

#endif // INSTALLERBASE_H

