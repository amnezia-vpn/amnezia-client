#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>
#include <QJsonObject>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(SshSession* sshSession, QObject *parent = nullptr);

    amnezia::ProtocolConfig createConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container, const amnezia::ContainerConfig &containerConfig,
                                const amnezia::DnsSettings &dnsSettings,
                                amnezia::ErrorCode &errorCode) override;

    amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                           amnezia::ProtocolConfig protocolConfig) override;

    amnezia::ErrorCode applyServerSettingsToRemote(const amnezia::ServerCredentials &credentials,
                                                   amnezia::DockerContainer container,
                                                   amnezia::ContainerConfig &containerConfig,
                                                   const amnezia::DnsSettings &dnsSettings,
                                                   bool appendNewClient,
                                                   QString *outClientId = nullptr);

    amnezia::ErrorCode writeServerConfigForSetup(const amnezia::ServerCredentials &credentials,
                                                 amnezia::DockerContainer container,
                                                 amnezia::ContainerConfig &containerConfig,
                                                 const amnezia::DnsSettings &dnsSettings);

    // Applies an edit of the server settings to the running server without reinstalling the container,
    // so the Reality keys, short ids and every issued client stay valid.
    amnezia::ErrorCode updateServerSettings(const amnezia::ServerCredentials &credentials,
                                            amnezia::DockerContainer container,
                                            const amnezia::ContainerConfig &oldConfig,
                                            amnezia::ContainerConfig &newConfig,
                                            const amnezia::DnsSettings &dnsSettings);

    static amnezia::XrayServerConfig mergeChangedSettings(const amnezia::XrayServerConfig &remote,
                                                          const amnezia::XrayServerConfig &oldSrv,
                                                          const amnezia::XrayServerConfig &newSrv);

    QJsonObject patchServerConfig(const QJsonObject &serverConfig, const amnezia::XrayServerConfig &current,
                                  const amnezia::XrayServerConfig &target, const QString &realityPrivateKey,
                                  const QString &realityShortId) const;

private:
    amnezia::ErrorCode readContainerKeyFile(amnezia::DockerContainer container,
                                            const amnezia::ServerCredentials &credentials,
                                            const QString &path, QString &out) const;

    amnezia::ErrorCode uploadServerConfigJson(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                                              const amnezia::DnsSettings &dnsSettings, const QJsonObject &serverConfig) const;

    amnezia::XrayProtocolConfig buildClientProtocolConfig(const amnezia::ServerCredentials &credentials,
                                                          amnezia::DockerContainer container,
                                                          const amnezia::XrayServerConfig &srv,
                                                          const QString &clientId,
                                                          amnezia::ErrorCode &errorCode,
                                                          const QString &prefetchedRealityPublicKey = {},
                                                          const QString &prefetchedRealityShortId = {}) const;

    amnezia::ErrorCode readRealityKeyFiles(amnezia::DockerContainer container,
                                           const amnezia::ServerCredentials &credentials,
                                           QString &outPublicKey,
                                           QString &outShortId) const;

    QJsonObject buildStreamSettings(const amnezia::XrayServerConfig &srv,
                                    const QString &clientId) const;
};

#endif // XRAY_CONFIGURATOR_H
