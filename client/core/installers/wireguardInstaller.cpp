#include "wireguardInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"

using namespace amnezia;
using namespace ProtocolUtils;

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
        wgConfig->serverConfig.subnetAddress = serverConfigMap.value("Address").remove("/24");
    }
    
    return ErrorCode::NoError;
}

