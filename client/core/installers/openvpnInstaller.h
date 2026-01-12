#ifndef OPENVPNINSTALLER_H
#define OPENVPNINSTALLER_H

#include "installerBase.h"

class OpenVpnInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit OpenVpnInstaller(QObject *parent = nullptr);

    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         SshSession* serverController, QJsonObject &config) override;
};

#endif // OPENVPNINSTALLER_H

