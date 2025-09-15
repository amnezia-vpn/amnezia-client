#ifndef AWGCONFIGURATOR_H
#define AWGCONFIGURATOR_H

#include <QObject>

#include "wireguard_configurator.h"

class AwgConfigurator : public WireguardConfigurator
{
    Q_OBJECT
public:
    AwgConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                ErrorCode &errorCode) override;
};

#endif // AWGCONFIGURATOR_H
