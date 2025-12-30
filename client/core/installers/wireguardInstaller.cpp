#include "wireguardInstaller.h"

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/controllers/serverController.h"

using namespace amnezia;

WireguardInstaller::WireguardInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode WireguardInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                         ServerController* serverController, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString serverConfig = serverController->getTextFileFromContainer(container, credentials,
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

    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    containerConfig[config_key::subnet_address] = serverConfigMap.value("Address").remove("/24");

    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

