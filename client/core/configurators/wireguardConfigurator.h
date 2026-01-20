#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QHostAddress>
#include <QObject>
#include <QProcessEnvironment>

#include "configuratorBase.h"
#include "core/utils/defs.h"
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

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container,
                                const ContainerConfig &containerConfig,
                                const DnsSettings &dnsSettings,
                                ErrorCode &errorCode) override;

    QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                           const SplitTunnelingSettings &splitTunneling,
                                           QString &protocolConfigString) override;
    QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                            QString &protocolConfigString) override;

    static ConnectionData genClientKeys();

private:
    QList<QHostAddress> getIpsFromConf(const QString &input);
    ConnectionData prepareWireguardConfig(const ServerCredentials &credentials, DockerContainer container,
                                          const WireGuardServerConfig* serverConfig,
                                          const AwgServerConfig* awgServerConfig,
                                          const DnsSettings &dnsSettings,
                                          ErrorCode &errorCode);

    bool m_isAwg;
    QString m_serverConfigPath;
    QString m_serverPublicKeyPath;
    QString m_serverPskKeyPath;
    amnezia::ProtocolScriptType m_configTemplate;
    QString m_protocolName;
    QString m_defaultPort;
};

#endif // WIREGUARD_CONFIGURATOR_H
