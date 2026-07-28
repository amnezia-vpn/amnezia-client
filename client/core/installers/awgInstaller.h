#ifndef AWGINSTALLER_H
#define AWGINSTALLER_H

#include "installerBase.h"

class AwgInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit AwgInstaller(QObject *parent = nullptr);

    amnezia::ContainerConfig generateConfig(amnezia::DockerContainer container, int port, amnezia::TransportProto transportProto) override;
    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;

private:
    void generateAwgParameters(amnezia::AwgServerConfig &serverConfig, bool isAwg2 = false);
};

#endif // AWGINSTALLER_H

