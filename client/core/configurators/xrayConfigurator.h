#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>
#include <QJsonArray>
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
                                                 const amnezia::DnsSettings &dnsSettings,
                                                 bool useAtomicApply = false);

    bool uploadClientTemplate(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                              const amnezia::XrayClientTemplate &clientTemplate) const;

    amnezia::XrayClientTemplate readClientTemplate(const amnezia::ServerCredentials &credentials,
                                                   amnezia::DockerContainer container, bool &outFound) const;

private:
    QJsonArray collectServerClients(const amnezia::ServerCredentials &credentials,
                                    amnezia::DockerContainer container, const QString &flowValue,
                                    const QString &fallbackClientId, amnezia::ErrorCode &outError) const;

    amnezia::ErrorCode uploadServerConfigAtomically(const amnezia::ServerCredentials &credentials,
                                                    amnezia::DockerContainer container, const QString &listenPort,
                                                    const QJsonObject &serverConfig) const;

    bool restartXrayContainer(const amnezia::ServerCredentials &credentials,
                              amnezia::DockerContainer container) const;

    bool xrayProcessIsUp(const amnezia::ServerCredentials &credentials,
                         amnezia::DockerContainer container) const;

    amnezia::ErrorCode readContainerKeyFile(amnezia::DockerContainer container,
                                            const amnezia::ServerCredentials &credentials,
                                            const QString &path, QString &out) const;

    QString prepareServerConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container, const amnezia::ContainerConfig &containerConfig,
                                const amnezia::DnsSettings &dnsSettings,
                                amnezia::ErrorCode &errorCode);

    amnezia::XrayProtocolConfig buildClientProtocolConfig(const amnezia::ServerCredentials &credentials,
                                                          amnezia::DockerContainer container,
                                                          const amnezia::XrayServerConfig &srv,
                                                          const amnezia::XrayClientTemplate &tpl,
                                                          const QString &clientId,
                                                          amnezia::ErrorCode &errorCode,
                                                          const QString &prefetchedRealityPublicKey = {},
                                                          const QString &prefetchedRealityShortId = {},
                                                          const QString &prefetchedTlsPin = {}) const;

    amnezia::ErrorCode readRealityKeyFiles(amnezia::DockerContainer container,
                                           const amnezia::ServerCredentials &credentials,
                                           QString &outPublicKey,
                                           QString &outShortId) const;

    amnezia::ErrorCode ensureTlsCertificate(const amnezia::ServerCredentials &credentials,
                                            amnezia::DockerContainer container,
                                            QString &outFingerprint) const;
};

#endif // XRAY_CONFIGURATOR_H
