#ifndef SFTPINSTALLER_H
#define SFTPINSTALLER_H

#include "installerBase.h"

class SftpInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit SftpInstaller(QObject *parent = nullptr);

    amnezia::ContainerConfig generateConfig(amnezia::DockerContainer container, int port, amnezia::TransportProto transportProto) override;
    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;
};

#endif // SFTPINSTALLER_H

