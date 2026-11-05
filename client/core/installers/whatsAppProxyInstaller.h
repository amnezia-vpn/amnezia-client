#ifndef WHATSAPPPROXYINSTALLER_H
#define WHATSAPPPROXYINSTALLER_H

#include "installerBase.h"

#include "core/models/protocols/whatsAppProxyProtocolConfig.h"

class WhatsAppProxyInstaller : public InstallerBase
{
    Q_OBJECT
public:
    explicit WhatsAppProxyInstaller(QObject *parent = nullptr);

    amnezia::ErrorCode extractConfigFromContainer(amnezia::DockerContainer container,
                                                  const amnezia::ServerCredentials &credentials, SshSession *sshSession,
                                                  amnezia::ContainerConfig &config) override;

    static void applyDockerPublishedPorts(const QString &dockerPsPortsLine, amnezia::WhatsAppProxyProtocolConfig &config);

    static void uploadClientSettingsSnapshot(SshSession &sshSession, const amnezia::ServerCredentials &credentials,
                                             amnezia::DockerContainer container, const amnezia::ContainerConfig &config);
};

#endif // WHATSAPPPROXYINSTALLER_H
