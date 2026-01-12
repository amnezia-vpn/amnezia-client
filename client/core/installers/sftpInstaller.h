#ifndef SFTPINSTALLER_H
#define SFTPINSTALLER_H

#include "installerBase.h"

class SftpInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit SftpInstaller(QObject *parent = nullptr);

    QJsonObject generateConfig(DockerContainer container, int port, TransportProto transportProto) override;
    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         SshSession* serverController, QJsonObject &config) override;
};

#endif // SFTPINSTALLER_H

