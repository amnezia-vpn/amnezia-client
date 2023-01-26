#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configurator_base.h"
#include "core/defs.h"

class WireguardConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    WireguardConfigurator(std::shared_ptr<Settings> settings,
                          std::shared_ptr<ServerController> serverController, QObject *parent = nullptr);

    struct ConnectionData {
        QString clientPrivKey; // client private key
        QString clientPubKey; // client public key
        QString clientIP; // internal client IP address
        QString serverPubKey; // tls-auth key
        QString pskKey; // preshared key
        QString host; // host ip
    };

    QString genWireguardConfig(const ServerCredentials &credentials, DockerContainer container,
                               const QJsonObject &containerConfig, ErrorCode &errorCode);

    QString processConfigWithLocalSettings(QString config);
    QString processConfigWithExportSettings(QString config);
    ErrorCode processLastConfigWithRemoteSettings(QMap<Proto, QString> &lastVpnConfigs, const int serverIndex);


private:
    ConnectionData prepareWireguardConfig(const ServerCredentials &credentials,
                                          DockerContainer container, const QJsonObject &containerConfig,
                                          ErrorCode &errorCode);

    ConnectionData genClientKeys();

signals:
    void remoteProcessingFinished();
};

#endif // WIREGUARD_CONFIGURATOR_H
