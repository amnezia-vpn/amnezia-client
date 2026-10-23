#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QHostAddress>
#include <QObject>
#include <QProcessEnvironment>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/selfhosted/scriptsRegistry.h"

class WireguardConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    WireguardConfigurator(SshSession* sshSession,
                          bool isAwg, QObject *parent = nullptr);

    struct ConnectionData
    {
        QString clientPrivKey; // client private key
        QString clientPubKey;  // client public key
        QString clientIP;      // internal client IP address
        QString serverPubKey;  // tls-auth key
        QString pskKey;        // preshared key
        QString host;          // host ip
        QString port;
    };

    amnezia::ProtocolConfig createConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                                const amnezia::ContainerConfig &containerConfig,
                                const amnezia::DnsSettings &dnsSettings,
                                amnezia::ErrorCode &errorCode) override;

    amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                           amnezia::ProtocolConfig protocolConfig) override;
    amnezia::ProtocolConfig processConfigWithExportSettings(const amnezia::ExportSettings &settings,
                                                            amnezia::ProtocolConfig protocolConfig) override;

    static ConnectionData genClientKeys();

private:
    QList<QHostAddress> getIpsFromConf(const QString &input);
    ConnectionData prepareWireguardConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container,
                                          const amnezia::WireGuardServerConfig* serverConfig,
                                          const amnezia::AwgServerConfig* awgServerConfig,
                                          const amnezia::DnsSettings &dnsSettings,
                                          amnezia::ErrorCode &errorCode);

    bool m_isAwg;
    QString m_serverConfigPath;
    QString m_serverPublicKeyPath;
    QString m_serverPskKeyPath;
    amnezia::ProtocolScriptType m_configTemplate;
    QString m_protocolName;
    QString m_defaultPort;
};

#endif // WIREGUARD_CONFIGURATOR_H
