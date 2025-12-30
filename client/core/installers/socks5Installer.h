#ifndef SOCKS5INSTALLER_H
#define SOCKS5INSTALLER_H

#include "installerBase.h"

class Socks5Installer : public InstallerBase
{
    Q_OBJECT
public:
    explicit Socks5Installer(QObject *parent = nullptr);

    QJsonObject generateConfig(DockerContainer container, int port, TransportProto transportProto) override;
    ErrorCode extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                         ServerController* serverController, QJsonObject &config) override;
};

#endif // SOCKS5INSTALLER_H

