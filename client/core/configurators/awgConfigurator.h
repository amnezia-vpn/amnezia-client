#ifndef AWGCONFIGURATOR_H
#define AWGCONFIGURATOR_H

#include <QObject>

#include "wireguardConfigurator.h"

class AwgConfigurator : public WireguardConfigurator
{
    Q_OBJECT
public:
    AwgConfigurator(SshSession* sshSession, QObject *parent = nullptr);

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container,
                                const ContainerConfig &containerConfig,
                                const DnsSettings &dnsSettings,
                                ErrorCode &errorCode) override;
};

#endif // AWGCONFIGURATOR_H
