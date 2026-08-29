#ifndef DNSTT_CONFIGURATOR_H
#define DNSTT_CONFIGURATOR_H

#include <QObject>
#include <QJsonObject>

#include "configuratorBase.h"
#include "core/models/protocols/dnsttProtocolConfig.h"

class DnsttConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    DnsttConfigurator(SshSession* sshSession, QObject *parent = nullptr);

    amnezia::ProtocolConfig createConfig(const amnezia::ServerCredentials &credentials,
                                         amnezia::DockerContainer container,
                                         const amnezia::ContainerConfig &containerConfig,
                                         const amnezia::DnsSettings &dnsSettings,
                                         amnezia::ErrorCode &errorCode) override;

    amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                           amnezia::ProtocolConfig protocolConfig) override;
};

#endif // DNSTT_CONFIGURATOR_H
