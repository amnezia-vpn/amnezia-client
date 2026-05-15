#ifndef MTPROXYINSTALLER_H
#define MTPROXYINSTALLER_H

#include "installerBase.h"

#include <QString>

struct MtProxyContainerDiagnostics {
    bool portReachable = false;
    bool upstreamReachable = false;
    int clientsConnected = -1;
    QString lastConfigRefresh;
    QString statsEndpoint;
};

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

    static amnezia::ErrorCode queryDiagnostics(SshSession &sshSession, const amnezia::ServerCredentials &credentials,
                                               amnezia::DockerContainer container, int listenPort,
                                               MtProxyContainerDiagnostics &out);
};

#endif // MTPROXYINSTALLER_H
