#include "installerBase.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;

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
    Proto protocol = ContainerUtils::defaultProtocol(container);
    QJsonObject containerConfig;

    containerConfig.insert(config_key::port, QString::number(port));
    containerConfig.insert(config_key::transport_proto, ProtocolUtils::transportProtoToString(transportProto, protocol));
    config.insert(config_key::container, ContainerUtils::containerToString(container));
    config.insert(ProtocolUtils::protoToString(protocol), containerConfig);
    
    return config;
}

