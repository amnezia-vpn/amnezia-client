#include "sftpInstaller.h"

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

SftpInstaller::SftpInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

QJsonObject SftpInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config = createBaseConfig(container, port, transportProto);
    
    auto mainProto = ContainerUtils::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolUtils::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
    containerConfig.insert(config_key::password, Utils::getRandomString(16));
    
    config.insert(ProtocolUtils::protoToString(mainProto), containerConfig);
    
    return config;
}

ErrorCode SftpInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                     SshSession* sshSession, QJsonObject &config)
{
    ErrorCode errorCode = ErrorCode::NoError;
    
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    QString containerName = ContainerUtils::containerToString(container);
    QString script = QString("sudo docker inspect --format '{{.Config.Cmd}}' %1").arg(containerName);

    errorCode = sshSession->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    auto sftpInfo = stdOut.split(":");
    if (sftpInfo.size() < 2) {
        return ErrorCode::ServerContainerMissingError;
    }

    auto mainProto = ContainerUtils::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolUtils::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::userName, sftpInfo.at(0).trimmed());
    containerConfig.insert(config_key::password, sftpInfo.at(1).trimmed());
    
    config.insert(ProtocolUtils::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

