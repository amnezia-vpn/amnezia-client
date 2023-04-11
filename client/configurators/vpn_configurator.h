#ifndef VPN_CONFIGURATOR_H
#define VPN_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/defs.h"


class OpenVpnConfigurator;
class ShadowSocksConfigurator;
class CloakConfigurator;
class WireguardConfigurator;
class Ikev2Configurator;
class SshConfigurator;

// Retrieve connection settings from server
class VpnConfigurator : ConfiguratorBase
{
    Q_OBJECT
public:
    VpnConfigurator(std::shared_ptr<Settings> settings,
        std::shared_ptr<ServerController> serverController, QObject *parent = nullptr);

    QString genVpnProtocolConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, Proto proto, ErrorCode *errorCode = nullptr);

    QPair<QString, QString> getDnsForConfig(int serverIndex);
    QString &processConfigWithDnsSettings(int serverIndex, DockerContainer container, Proto proto, QString &config);

    QString &processConfigWithLocalSettings(int serverIndex, DockerContainer container, Proto proto, QString &config);
    QString &processConfigWithExportSettings(int serverIndex, DockerContainer container, Proto proto, QString &config);

    // workaround for containers which is not support normal configuration
    void updateContainerConfigAfterInstallation(DockerContainer container,
        QJsonObject &containerConfig, const QString &stdOut);

    std::shared_ptr<ServerController> m_serverController;

    std::shared_ptr<OpenVpnConfigurator> openVpnConfigurator;
    std::shared_ptr<ShadowSocksConfigurator> shadowSocksConfigurator;
    std::shared_ptr<CloakConfigurator> cloakConfigurator;
    std::shared_ptr<WireguardConfigurator> wireguardConfigurator;
    std::shared_ptr<Ikev2Configurator> ikev2Configurator;
    std::shared_ptr<SshConfigurator> sshConfigurator;
};

#endif // VPN_CONFIGURATOR_H
