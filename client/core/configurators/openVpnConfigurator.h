#ifndef OPENVPN_CONFIGURATOR_H
#define OPENVPN_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class OpenVpnConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    OpenVpnConfigurator(SshSession* sshSession, QObject *parent = nullptr);

    struct ConnectionData
    {
        QString clientId;
        QString request;    // certificate request
        QString privKey;    // client private key
        QString clientCert; // client signed certificate
        QString caCert;     // server certificate
        QString taKey;      // tls-auth key
        QString host;       // host ip
    };

    amnezia::ProtocolConfig createConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                               const amnezia::ContainerConfig &containerConfig,
                               const amnezia::DnsSettings &dnsSettings,
                               amnezia::ErrorCode &errorCode) override;

    amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                           amnezia::ProtocolConfig protocolConfig) override;
    amnezia::ProtocolConfig processConfigWithExportSettings(const amnezia::ExportSettings &settings,
                                                            amnezia::ProtocolConfig protocolConfig) override;

    static ConnectionData createCertRequest();

private:
    ConnectionData prepareOpenVpnConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                                       const amnezia::DnsSettings &dnsSettings,
                                       amnezia::ErrorCode &errorCode);
    amnezia::ErrorCode signCert(amnezia::DockerContainer container, const amnezia::ServerCredentials &credentials, 
                      const amnezia::DnsSettings &dnsSettings, QString clientId);
};

#endif // OPENVPN_CONFIGURATOR_H
