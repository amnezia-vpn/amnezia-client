#ifndef WIREGUARD_CONFIGURATOR_H
#define WIREGUARD_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class WireguardConfigurator
{
public:

    struct ConnectionData {
        QString clientPrivKey; // client private key
        QString clientPubKey; // client public key
        QString serverPubKey; // tls-auth key
        QString pskKey; // preshared key
        QString host; // host ip
    };

    static QString genWireguardConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);

    static QString processConfigWithLocalSettings(QString config);
    static QString processConfigWithExportSettings(QString config);


private:
    static QProcessEnvironment prepareEnv();

    static ConnectionData prepareWireguardConfig(const ServerCredentials &credentials,
        DockerContainer container, ErrorCode *errorCode = nullptr);

    static ConnectionData genClientKeys();

    static Settings &m_settings();
};

#endif // WIREGUARD_CONFIGURATOR_H
