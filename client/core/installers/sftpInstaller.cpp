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
#include "core/models/protocols/sftpProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

SftpInstaller::SftpInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ContainerConfig SftpInstaller::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    ContainerConfig config = createBaseConfig(container, port, transportProto);
    
    if (auto* sftpConfig = config.getSftpProtocolConfig()) {
        sftpConfig->userName = protocols::sftp::defaultUserName;
        sftpConfig->password = Utils::getRandomString(16);
    }
    
    return config;
}

ErrorCode SftpInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                     SshSession* sshSession, ContainerConfig &config)
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

    if (auto* sftpConfig = config.getSftpProtocolConfig()) {
        sftpConfig->userName = sftpInfo.at(0).trimmed();
        sftpConfig->password = sftpInfo.at(1).trimmed();
    }
    
    return ErrorCode::NoError;
}

