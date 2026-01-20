#ifndef OPENVPN_CONFIGURATOR_H
#define OPENVPN_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configuratorBase.h"
#include "core/utils/defs.h"

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

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container,
                               const ContainerConfig &containerConfig,
                               const DnsSettings &dnsSettings,
                               ErrorCode &errorCode) override;

    QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                           const SplitTunnelingSettings &splitTunneling,
                                           QString &protocolConfigString) override;
    QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                            QString &protocolConfigString) override;

    static ConnectionData createCertRequest();

private:
    ConnectionData prepareOpenVpnConfig(const ServerCredentials &credentials, DockerContainer container,
                                       const DnsSettings &dnsSettings,
                                       ErrorCode &errorCode);
    ErrorCode signCert(DockerContainer container, const ServerCredentials &credentials, 
                      const DnsSettings &dnsSettings, QString clientId);
};

#endif // OPENVPN_CONFIGURATOR_H
