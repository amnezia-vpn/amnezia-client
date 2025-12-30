#include "openvpnInstaller.h"

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/controllers/serverController.h"

using namespace amnezia;

OpenVpnInstaller::OpenVpnInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode OpenVpnInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                       ServerController* serverController, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString serverConfig = serverController->getTextFileFromContainer(container, credentials,
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

    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();

    QString serverValue = serverConfigMap.value("server");

    if (!serverValue.isEmpty()) {
        QStringList serverParts = serverValue.split(" ");
        if (serverParts.count() >= 1) {
            containerConfig[config_key::subnet_address] = serverParts[0];
        }
    }

    bool ncpDisable = serverConfig.contains("ncp-disable");
    containerConfig[config_key::ncp_disable] = ncpDisable;

    bool tlsAuth = serverConfig.contains("tls-auth");
    containerConfig[config_key::tls_auth] = tlsAuth;

    bool blockOutsideDns = serverConfig.contains("block-outside-dns");
    containerConfig[config_key::block_outside_dns] = blockOutsideDns;

    QString cipher = serverConfigMap.value("cipher");
    if (!cipher.isEmpty()) {
        containerConfig[config_key::cipher] = cipher;
    }

    QString hash = serverConfigMap.value("auth");
    if (!hash.isEmpty()) {
        containerConfig[config_key::hash] = hash;
    }

    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

