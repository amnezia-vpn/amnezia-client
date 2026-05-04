#ifndef MTPROXYINSTALLER_H
#define MTPROXYINSTALLER_H

#include "installerBase.h"

class MtProxyInstaller : public InstallerBase {
Q_OBJECT
public:
    explicit MtProxyInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode
    extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                               SshSession *sshSession, amnezia::ContainerConfig &config) override;

    static void uploadClientSettingsSnapshot(SshSession &sshSession, const amnezia::ServerCredentials &credentials,
                                             amnezia::DockerContainer container,
                                             const amnezia::ContainerConfig &config);
};

#endif // MTPROXYINSTALLER_H
