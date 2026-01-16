#ifndef AWGCONFIGURATOR_H
#define AWGCONFIGURATOR_H

#include <QObject>

#include "wireguardConfigurator.h"

class AwgConfigurator : public WireguardConfigurator
{
    Q_OBJECT
public:
    AwgConfigurator(AppSettingsRepository* appSettingsRepository, SshSession* sshSession, QObject *parent = nullptr);

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container,
                                const ContainerConfig &containerConfig, ErrorCode &errorCode);
};

#endif // AWGCONFIGURATOR_H
