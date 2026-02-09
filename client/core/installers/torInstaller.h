#ifndef TORINSTALLER_H
#define TORINSTALLER_H

#include "installerBase.h"

class TorInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit TorInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;
};

#endif // TORINSTALLER_H

