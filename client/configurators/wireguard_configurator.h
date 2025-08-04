#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QHostAddress>
#include <QObject>
#include <QProcessEnvironment>

#include "configurator_base.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/defs.h"
#include "core/scripts_registry.h"

class WireguardConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    WireguardConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
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

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) override;

    QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                           QString &protocolConfigString);
    QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                            QString &protocolConfigString);

    Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                             const QSharedPointer<ProtocolConfig> &protocolConfig) const override;

    static ConnectionData genClientKeys();

private:
    QList<QHostAddress> getIpsFromConf(const QString &input);
    ConnectionData prepareWireguardConfig(const ServerCredentials &credentials, DockerContainer container,
                                          const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode);

    bool m_isAwg;
    QString m_serverConfigPath;
    QString m_serverPublicKeyPath;
    QString m_serverPskKeyPath;
    amnezia::ProtocolScriptType m_configTemplate;
    QString m_protocolName;
    QString m_defaultPort;
};

#endif // WIREGUARD_CONFIGURATOR_H
