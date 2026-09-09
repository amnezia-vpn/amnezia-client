#include "wireguardInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"

#include <QAbstractSocket>
#include <QHostAddress>

using namespace amnezia;
using namespace ProtocolUtils;

namespace
{
void parseInterfaceAddresses(const QString &addressValue, WireGuardServerConfig &serverConfig)
{
    const QStringList addresses = addressValue.split(",", Qt::SkipEmptyParts);
    for (const QString &addressWithPrefix : addresses) {
        const QString trimmed = addressWithPrefix.trimmed();
        const QString address = trimmed.section("/", 0, 0).trimmed();
        const QString prefix = trimmed.section("/", 1, 1).trimmed();
        const QHostAddress hostAddress(address);
        if (hostAddress.protocol() == QAbstractSocket::IPv4Protocol) {
            serverConfig.subnetAddress = address;
            serverConfig.subnetCidr = prefix;
        } else if (hostAddress.protocol() == QAbstractSocket::IPv6Protocol) {
            serverConfig.subnetIpv6Address = address;
            serverConfig.subnetIpv6Cidr = prefix;
        }
    }
}
}

WireguardInstaller::WireguardInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode WireguardInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                         SshSession* sshSession, ContainerConfig &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    
    QString serverConfig = sshSession->getTextFileFromContainer(container, credentials,
                                                                      protocols::wireguard::serverConfigPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QMap<QString, QString> serverConfigMap;
    auto serverConfigLines = serverConfig.split("\n");
    for (auto &line : serverConfigLines) {
        auto trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("[") && trimmedLine.endsWith("]")) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" = ");
            if (parts.count() == 2) {
                serverConfigMap.insert(parts[0].trimmed(), parts[1].trimmed());
            }
        }
    }

    if (auto* wgConfig = config.getWireGuardProtocolConfig()) {
        parseInterfaceAddresses(serverConfigMap.value("Address"), wgConfig->serverConfig);
    }
    
    return ErrorCode::NoError;
}
