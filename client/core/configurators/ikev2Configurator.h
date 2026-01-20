#ifndef IKEV2_CONFIGURATOR_H
#define IKEV2_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configuratorBase.h"
#include "core/utils/defs.h"

class Ikev2Configurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    Ikev2Configurator(SshSession* sshSession, QObject *parent = nullptr);

    struct ConnectionData {
        QByteArray clientCert; // p12 client cert
        QByteArray caCert; // p12 server cert
        QString clientId;
        QString password; // certificate password
        QString host; // host ip
    };

    ProtocolConfig createConfig(const ServerCredentials &credentials, DockerContainer container,
                                const ContainerConfig &containerConfig,
                                const DnsSettings &dnsSettings,
                                ErrorCode &errorCode) override;

    QString genIkev2Config(const ConnectionData &connData);
    QString genMobileConfig(const ConnectionData &connData);
    QString genStrongSwanConfig(const ConnectionData &connData);

    ConnectionData prepareIkev2Config(const ServerCredentials &credentials,
        DockerContainer container, ErrorCode &errorCode);
};

#endif // IKEV2_CONFIGURATOR_H
