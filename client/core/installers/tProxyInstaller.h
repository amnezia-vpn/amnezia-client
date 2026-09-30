#ifndef TPROXYINSTALLER_H
#define TPROXYINSTALLER_H

#include "installerBase.h"

#include "core/models/protocols/tProxyProtocolConfig.h"

class TProxyInstaller : public InstallerBase {
Q_OBJECT
public:
    explicit TProxyInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode
    extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                               SshSession *sshSession, amnezia::ContainerConfig &config) override;

    static void applyDockerPublishedPorts(const QString &dockerPsPortsLine,
                                          amnezia::TProxyProtocolConfig &config);

    static void uploadClientSettingsSnapshot(SshSession &sshSession, const amnezia::ServerCredentials &credentials,
                                             amnezia::DockerContainer container,
                                             const amnezia::ContainerConfig &config);
};

#endif // TPROXYINSTALLER_H
