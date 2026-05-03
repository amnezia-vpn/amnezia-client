#include "torInstaller.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/models/protocols/torProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

TorInstaller::TorInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode TorInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
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
    QString script = QString("sudo docker exec -i %1 sh -c 'cat /var/lib/tor/hidden_service/hostname'").arg(containerName);

    errorCode = sshSession->runScript(credentials, script, cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (stdOut.isEmpty()) {
        return ErrorCode::ServerContainerMissingError;
    }

    QString onion = stdOut;
    onion.replace("\n", "");

    if (auto* torConfig = config.getTorProtocolConfig()) {
        torConfig->serverConfig.site = onion;
    }
    
    return ErrorCode::NoError;
}

