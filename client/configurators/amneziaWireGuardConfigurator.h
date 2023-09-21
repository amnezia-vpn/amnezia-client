#ifndef AMNEZIAWIREGUARDCONFIGURATOR_H
#define AMNEZIAWIREGUARDCONFIGURATOR_H

#include <QObject>

#include "wireguard_configurator.h"

class AmneziaWireGuardConfigurator : public WireguardConfigurator
{
    Q_OBJECT
public:
    AmneziaWireGuardConfigurator(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QString genAmneziaWireGuardConfig(const ServerCredentials &credentials, DockerContainer container,
                                      const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);
};

#endif // AMNEZIAWIREGUARDCONFIGURATOR_H
