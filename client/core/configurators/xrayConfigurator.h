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

private:
    QString prepareServerConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container, const amnezia::ContainerConfig &containerConfig,
                                const amnezia::DnsSettings &dnsSettings,
                                amnezia::ErrorCode &errorCode);

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

    QJsonObject mergeStreamSettingsForServerInbound(const amnezia::XrayServerConfig &srv,
                                                    const QJsonObject &existingStreamSettings) const;

    QJsonObject buildStreamSettings(const amnezia::XrayServerConfig &srv,
                                    const QString &clientId) const;
};

#endif // XRAY_CONFIGURATOR_H
