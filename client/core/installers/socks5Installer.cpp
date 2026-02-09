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

ContainerConfig Socks5Installer::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    ContainerConfig config = createBaseConfig(container, port, transportProto);
    
    if (auto* socks5Config = config.getSocks5ProxyProtocolConfig()) {
        socks5Config->userName = protocols::socks5Proxy::defaultUserName;
        socks5Config->password = Utils::getRandomString(16);
    }
    
    return config;
}

ErrorCode Socks5Installer::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                       SshSession* sshSession, ContainerConfig &config)
{
    Q_UNUSED(container);
    Q_UNUSED(credentials);
    Q_UNUSED(sshSession);
    Q_UNUSED(config);
    return ErrorCode::NoError;
}

