#ifndef XRAYINSTALLER_H
#define XRAYINSTALLER_H

#include <QJsonObject>

#include "installerBase.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit XrayInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials,
                                         SshSession* serverController, amnezia::ContainerConfig &config) override;

    // Fills the settings the server actually runs with from its server.json; fields it does not describe are kept.
    static amnezia::ErrorCode readServerConfig(const QJsonObject &serverConfig, amnezia::XrayServerConfig &srv);
};

#endif // XRAYINSTALLER_H

