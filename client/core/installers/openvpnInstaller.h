#ifndef OPENVPNINSTALLER_H
#define OPENVPNINSTALLER_H

#include "installerBase.h"

class OpenVpnInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit OpenVpnInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;
};

#endif // OPENVPNINSTALLER_H

