#ifndef SOCKS5INSTALLER_H
#define SOCKS5INSTALLER_H

#include "installerBase.h"

class Socks5Installer : public InstallerBase
{
    Q_OBJECT
public:
    explicit Socks5Installer(QObject *parent = nullptr);

    amnezia::ContainerConfig generateConfig(amnezia::DockerContainer container, int port, amnezia::TransportProto transportProto) override;
    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;
};

#endif // SOCKS5INSTALLER_H

