#include "socks5Installer.h"

#include "core/models/protocols/socks5ProxyProtocolConfig.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/utilities.h"

#include <QRegularExpression>

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
    if (container != DockerContainer::Socks5Proxy || !sshSession) {
        return ErrorCode::NoError;
    }

    Socks5ProxyProtocolConfig *socks5Config = config.getSocks5ProxyProtocolConfig();
    if (!socks5Config) {
        return ErrorCode::NoError;
    }

    ErrorCode readError = ErrorCode::NoError;
    const QByteArray configRaw = sshSession->getTextFileFromContainer(
            container, credentials, QString::fromUtf8(protocols::socks5Proxy::proxyConfigPath), readError);
    if (readError != ErrorCode::NoError || configRaw.trimmed().isEmpty()) {
        return ErrorCode::NoError;
    }

    const QString proxyConfig = QString::fromUtf8(configRaw);
    static const QRegularExpression usernameAndPasswordRegExp(QStringLiteral("users (\\w+):CL:(\\w+)"));
    const QRegularExpressionMatch usernameAndPasswordMatch = usernameAndPasswordRegExp.match(proxyConfig);
    if (usernameAndPasswordMatch.hasMatch()) {
        socks5Config->userName = usernameAndPasswordMatch.captured(1);
        socks5Config->password = usernameAndPasswordMatch.captured(2);
    }

    return ErrorCode::NoError;
}
