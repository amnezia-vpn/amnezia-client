#ifndef WIREGUARDINSTALLER_H
#define WIREGUARDINSTALLER_H

#include "installerBase.h"

class WireguardInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit WireguardInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;
};

#endif // WIREGUARDINSTALLER_H

