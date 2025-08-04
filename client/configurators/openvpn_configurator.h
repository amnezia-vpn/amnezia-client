#ifndef OPENVPN_CONFIGURATOR_H
#define OPENVPN_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configurator_base.h"
#include "core/defs.h"
#include "core/models/protocols/openvpnProtocolConfig.h"

class OpenVpnConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    OpenVpnConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

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

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) override;

    QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                           QString &protocolConfigString);
    QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                            QString &protocolConfigString);

    Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                             const QSharedPointer<ProtocolConfig> &protocolConfig) const override;

    static ConnectionData createCertRequest();

private:
    ConnectionData prepareOpenVpnConfig(const ServerCredentials &credentials, DockerContainer container,
                                        ErrorCode &errorCode);
    ErrorCode signCert(DockerContainer container, const ServerCredentials &credentials, QString clientId);
};

#endif // OPENVPN_CONFIGURATOR_H
