#ifndef OPENVPN_CONFIGURATOR_H
#define OPENVPN_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configurator_base.h"
#include "core/defs.h"

class OpenVpnConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    OpenVpnConfigurator(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

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

    QString createConfig(const ServerCredentials &credentials, DockerContainer container,
                         const QJsonObject &containerConfig, ErrorCode errorCode);

    QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                           QString &protocolConfigString);
    QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                            QString &protocolConfigString);

    static ConnectionData createCertRequest();

private:
    ConnectionData prepareOpenVpnConfig(const ServerCredentials &credentials, DockerContainer container,
                                        ErrorCode errorCode);
    ErrorCode signCert(DockerContainer container, const ServerCredentials &credentials, QString clientId);
};

#endif // OPENVPN_CONFIGURATOR_H
