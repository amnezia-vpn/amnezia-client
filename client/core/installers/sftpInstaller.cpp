#include "sftpInstaller.h"

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/utils/selfhosted/sshSession.h"
#include "utilities.h"

using namespace amnezia;

SftpInstaller::SftpInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

QJsonObject SftpInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    QJsonObject config = createBaseConfig(container, port, transportProto);
    
    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
    containerConfig.insert(config_key::password, Utils::getRandomString(16));
    
    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
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

    QString containerName = ContainerProps::containerToString(container);
    QString script = QString("sudo docker inspect --format '{{.Config.Cmd}}' %1").arg(containerName);

    errorCode = sshSession->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    auto sftpInfo = stdOut.split(":");
    if (sftpInfo.size() < 2) {
        return ErrorCode::ServerContainerMissingError;
    }

    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::userName, sftpInfo.at(0).trimmed());
    containerConfig.insert(config_key::password, sftpInfo.at(1).trimmed());
    
    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

