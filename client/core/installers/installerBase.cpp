#include "installerBase.h"

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;

InstallerBase::InstallerBase(QObject *parent)
    : QObject(parent)
{
}

QJsonObject InstallerBase::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    return createBaseConfig(container, port, transportProto);
}

ErrorCode InstallerBase::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                   SshSession* sshSession, QJsonObject &config)
{
    Q_UNUSED(container);
    Q_UNUSED(credentials);
    Q_UNUSED(sshSession);
    Q_UNUSED(config);
    return ErrorCode::NoError;
}

QJsonObject InstallerBase::createBaseConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config;
    auto mainProto = ContainerProps::defaultProtocol(container);
    
    for (auto protocol : ContainerProps::protocolsForContainer(container)) {
        QJsonObject containerConfig;

        if (protocol == mainProto) {
            containerConfig.insert(config_key::port, QString::number(port));
            containerConfig.insert(config_key::transport_proto, ProtocolProps::transportProtoToString(transportProto, protocol));
            config.insert(config_key::container, ContainerProps::containerToString(container));
        }
        config.insert(ProtocolProps::protoToString(protocol), containerConfig);
    }
    
    return config;
}

