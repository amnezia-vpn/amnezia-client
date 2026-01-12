#include "socks5Installer.h"

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/utils/selfhosted/sshSession.h"
#include "utilities.h"

using namespace amnezia;

Socks5Installer::Socks5Installer(QObject *parent)
    : InstallerBase(parent)
{
}

QJsonObject Socks5Installer::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config = createBaseConfig(container, port, transportProto);
    
    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::userName, protocols::socks5Proxy::defaultUserName);
    containerConfig.insert(config_key::password, Utils::getRandomString(16));
    
    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
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

