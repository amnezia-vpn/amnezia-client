#ifndef OPENVPN_CONFIGURATOR_H
#define OPENVPN_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

class OpenVpnConfigurator
{
public:

    struct ConnectionData {
        QString clientId;
        QString request; // certificate request
        QString privKey; // client private key
        QString clientCert; // client signed certificate
        QString caCert; // server certificate
        QString taKey; // tls-auth key
        QString host; // host ip
    };

    static QString genOpenVpnConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);

    static QString processConfigWithLocalSettings(QString config);
    static QString processConfigWithExportSettings(QString config);

    static QString convertOpenSShKey(const QString &key);

    static ErrorCode signCert(DockerContainer container,
        const ServerCredentials &credentials, QString clientId);

private:
    static QString getEasyRsaShPath();

    static QProcessEnvironment prepareEnv();
    static ErrorCode initPKI(const QString &path);
    static ErrorCode genReq(const QString &path, const QString &clientId);

    static ConnectionData createCertRequest();

    static ConnectionData prepareOpenVpnConfig(const ServerCredentials &credentials,
        DockerContainer container, ErrorCode *errorCode = nullptr);

    static Settings &m_settings();
};

#endif // OPENVPN_CONFIGURATOR_H
