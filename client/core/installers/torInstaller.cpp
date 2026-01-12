#include "torInstaller.h"

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/utils/selfhosted/sshSession.h"

using namespace amnezia;

TorInstaller::TorInstaller(QObject *parent)
    : InstallerBase(parent)
{
}

ErrorCode TorInstaller::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
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

    auto mainProto = ContainerProps::defaultProtocol(container);
    QJsonObject containerConfig = config.value(ProtocolProps::protoToString(mainProto)).toObject();
    
    containerConfig.insert(config_key::site, onion);

    config.insert(ProtocolProps::protoToString(mainProto), containerConfig);
    
    return ErrorCode::NoError;
}

