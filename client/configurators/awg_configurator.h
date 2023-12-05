#ifndef AWGCONFIGURATOR_H
#define AWGCONFIGURATOR_H

#include <QObject>

#include "wireguard_configurator.h"

class AwgConfigurator : public WireguardConfigurator
{
    Q_OBJECT
public:
    AwgConfigurator(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QString genAwgConfig(const ServerCredentials &credentials, DockerContainer container,
                         const QJsonObject &containerConfig, QString &clientId, ErrorCode *errorCode = nullptr);
};

#endif // AWGCONFIGURATOR_H
