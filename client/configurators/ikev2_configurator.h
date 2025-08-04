#ifndef IKEV2_CONFIGURATOR_H
#define IKEV2_CONFIGURATOR_H

#include <QObject>
#include <QProcessEnvironment>

#include "configurator_base.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"
#include "core/defs.h"

class Ikev2Configurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    Ikev2Configurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    struct ConnectionData {
        QByteArray clientCert; // p12 client cert
        QByteArray caCert; // p12 server cert
        QString clientId;
        QString password; // certificate password
        QString host; // host ip
    };

    QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) override;

    QString genIkev2Config(const ConnectionData &connData);
    QString genMobileConfig(const ConnectionData &connData);
    QString genStrongSwanConfig(const ConnectionData &connData);

    Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                             const QSharedPointer<ProtocolConfig> &protocolConfig) const override;

    ConnectionData prepareIkev2Config(const ServerCredentials &credentials,
        DockerContainer container, ErrorCode &errorCode);
};

#endif // IKEV2_CONFIGURATOR_H
