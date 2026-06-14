#include "openvpnInstaller.h"

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

OpenVpnInstaller::OpenVpnInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode OpenVpnInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                       SshSession* sshSession, ContainerConfig &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    
    QString serverConfig = sshSession->getTextFileFromContainer(container, credentials,
                                                                      protocols::openvpn::serverConfigPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QMap<QString, QString> serverConfigMap;
    auto serverConfigLines = serverConfig.split("\n");
    for (auto &line : serverConfigLines) {
        auto trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("#") || trimmedLine.isEmpty()) {
            continue;
        } else {
            QStringList parts = trimmedLine.split(" ");
            if (parts.count() >= 2) {
                QString key = parts[0];
                QString value = parts.mid(1).join(" ");
                serverConfigMap.insert(key, value);
            }
        }
    }

    if (auto* ovpnConfig = config.getOpenVpnProtocolConfig()) {
        QString serverValue = serverConfigMap.value("server");

        if (!serverValue.isEmpty()) {
            QStringList serverParts = serverValue.split(" ");
            if (serverParts.count() >= 1) {
                ovpnConfig->serverConfig.subnetAddress = serverParts[0];
            }
        }

        ovpnConfig->serverConfig.ncpDisable = serverConfig.contains("ncp-disable");
        ovpnConfig->serverConfig.tlsAuth = serverConfig.contains("tls-auth");

        QString cipher = serverConfigMap.value("cipher");
        if (!cipher.isEmpty()) {
            ovpnConfig->serverConfig.cipher = cipher;
        }

        QString hash = serverConfigMap.value("auth");
        if (!hash.isEmpty()) {
            ovpnConfig->serverConfig.hash = hash;
        }
    }
    
    return ErrorCode::NoError;
}

