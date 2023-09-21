#include "amneziaWireGuardConfigurator.h"

AmneziaWireGuardConfigurator::AmneziaWireGuardConfigurator(std::shared_ptr<Settings> settings, QObject *parent)
    : WireguardConfigurator(settings, parent)
{
}

QString AmneziaWireGuardConfigurator::genAmneziaWireGuardConfig(const ServerCredentials &credentials,
                                                                DockerContainer container,
                                                                const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    auto config = WireguardConfigurator::genWireguardConfig(credentials, container, containerConfig, errorCode);

    return config;
}
