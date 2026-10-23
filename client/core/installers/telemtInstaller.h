#ifndef TELEMTINSTALLER_H
#define TELEMTINSTALLER_H

#include "installerBase.h"

class TelemtInstaller : public InstallerBase {
Q_OBJECT
public:
    explicit TelemtInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode
    extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                               SshSession *sshSession, amnezia::ContainerConfig &config) override;

    static void uploadClientSettingsSnapshot(SshSession &sshSession, const amnezia::ServerCredentials &credentials,
                                             amnezia::DockerContainer container,
                                             const amnezia::ContainerConfig &config);
};

#endif // TELEMTINSTALLER_H
