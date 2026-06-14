#ifndef XRAYINSTALLER_H
#define XRAYINSTALLER_H

#include "installerBase.h"

class XrayInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit XrayInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;
};

#endif // XRAYINSTALLER_H

