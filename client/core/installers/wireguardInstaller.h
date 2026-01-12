#ifndef WIREGUARDINSTALLER_H
#define WIREGUARDINSTALLER_H

#include "installerBase.h"

class WireguardInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit WireguardInstaller(QObject *parent = nullptr);

    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         SshSession* serverController, QJsonObject &config) override;
};

#endif // WIREGUARDINSTALLER_H

