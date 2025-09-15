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
    OpenVpnConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                        QObject *parent = nullptr);

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

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                ErrorCode &errorCode) override;

    QSharedPointer<ProtocolConfig> processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                                  QSharedPointer<ProtocolConfig> protocolConfig) override;
    QSharedPointer<ProtocolConfig> processConfigWithExportSettings(const QPair<QString, QString> &dns,
                                                                   QSharedPointer<ProtocolConfig> protocolConfig) override;

    static ConnectionData createCertRequest();

private:
    ConnectionData prepareOpenVpnConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                        ErrorCode &errorCode);
    ErrorCode signCert(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig, QString clientId);
};

#endif // OPENVPN_CONFIGURATOR_H
