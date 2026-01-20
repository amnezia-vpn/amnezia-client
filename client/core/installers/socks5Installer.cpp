#include "socks5Installer.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/utilities.h"

using namespace amnezia;
using namespace ProtocolUtils;

Socks5Installer::Socks5Installer(QObject *parent)
    : InstallerBase(parent)
{
}

QJsonObject Socks5Installer::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config = createBaseConfig(container, port, transportProto);
    
    auto mainProto = ContainerUtils::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolUtils::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::userName, protocols::socks5Proxy::defaultUserName);
    containerConfig.insert(config_key::password, Utils::getRandomString(16));
    
    config.insert(ProtocolUtils::protoToString(mainProto), containerConfig);
    
    return config;
}

ErrorCode Socks5Installer::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                       SshSession* sshSession, QJsonObject &config)
{
    Q_UNUSED(container);
    Q_UNUSED(credentials);
    Q_UNUSED(sshSession);
    Q_UNUSED(config);
    return ErrorCode::NoError;
}

