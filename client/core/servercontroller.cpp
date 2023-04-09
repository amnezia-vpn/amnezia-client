#include "servercontroller.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QApplication>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QThread>
#include <QtConcurrent>

#include "sftpchannel.h"
#include "sshconnectionmanager.h"

#include "containers/containers_defs.h"
#include "server_defs.h"
#include "settings.h"
#include "scripts_registry.h"
#include "utilities.h"

#include <configurators/vpn_configurator.h>

using namespace QSsh;

ServerController::ServerController(std::shared_ptr<Settings> settings, QObject *parent) :
    m_settings(settings)
{

}

ErrorCode ServerController::runScript(const ServerCredentials &credentials, QString script,
    const std::function<void(const QString &, QSharedPointer<SshRemoteProcess>)> &cbReadStdOut,
    const std::function<void(const QString &, QSharedPointer<SshRemoteProcess>)> &cbReadStdErr)
{
    QSharedPointer<SshConnection> client = connectToHost(sshParams(credentials));
    if (client->state() == SshConnection::State::Connecting) {
        qDebug() << "ServerController::runScript aborted, connectToHost in progress";
        return ErrorCode::SshTimeoutError;
    }
    else if (client->state() != SshConnection::State::Connected) {
        qDebug() << "ServerController::runScript connectToHost error: " << fromSshConnectionErrorCode(client->errorState());
        return fromSshConnectionErrorCode(client->errorState());
    }

    script.replace("\r", "");

    qDebug() << "Run script";

    QString totalLine;
    const QStringList &lines = script.split("\n", Qt::SkipEmptyParts);
    for (int i = 0; i < lines.count(); i++) {
        QString currentLine = lines.at(i);
        QString nextLine;
        if (i + 1 < lines.count()) nextLine = lines.at(i+1);

        if (totalLine.isEmpty()) {
            totalLine = currentLine;
        }
        else {
            totalLine = totalLine + "\n" + currentLine;
        }

        QString lineToExec;
        if (currentLine.endsWith("\\")) {
            continue;
        }
        else {
            lineToExec = totalLine;
            totalLine.clear();
        }

        // Run collected line
        if (totalLine.startsWith("#")) {
            continue;
        }

        qDebug().noquote() << "EXEC" << lineToExec;
        Logger::appendSshLog("Run command:" + lineToExec);

        QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(lineToExec.toUtf8());

        if (!proc) {
            qCritical() << "Failed to create SshRemoteProcess, breaking.";
            return ErrorCode::SshRemoteProcessCreationError;
        }

        QEventLoop wait;
        int exitStatus = -1;

        //        QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
        //            qDebug() << "Command started";
        //        });

        QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
            exitStatus = status;
            //qDebug() << "Remote process exited with status" << status;
            wait.quit();
        });

        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, &wait, [proc, cbReadStdOut](){
            QString s = proc->readAllStandardOutput();

            if (s != "." && !s.isEmpty()) {
                Logger::appendSshLog("Output: " + s);
                qDebug().noquote() << "stdout" << s;
            }
            if (cbReadStdOut) cbReadStdOut(s, proc);
        });

        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, &wait, [proc, cbReadStdErr](){
            QString s = proc->readAllStandardError();
            if (s != "." && !s.isEmpty()) {
                Logger::appendSshLog("Output: " + s);
                qDebug().noquote() << "stderr" << s;
            }
            if (cbReadStdErr) cbReadStdErr(s, proc);
        });

        proc->start();
        if (i < lines.count() && exitStatus < 0) {
            wait.exec();
        }

        if (SshRemoteProcess::ExitStatus(exitStatus) != QSsh::SshRemoteProcess::ExitStatus::NormalExit) {
            return fromSshProcessExitStatus(exitStatus);
        }
    }

    qDebug() << "ServerController::runScript finished\n";
    return ErrorCode::NoError;
}

ErrorCode ServerController::runContainerScript(const ServerCredentials &credentials,
    DockerContainer container, QString script,
    const std::function<void (const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdOut,
    const std::function<void (const QString &, QSharedPointer<QSsh::SshRemoteProcess>)> &cbReadStdErr)
{
    QString fileName = "/opt/amnezia/" + Utils::getRandomString(16) + ".sh";
    Logger::appendSshLog("Run container script for " + ContainerProps::containerToString(container) + ":\n" + script);

    ErrorCode e = uploadTextFileToContainer(container, credentials, script, fileName);
    if (e) return e;

    QString runner = QString("sudo docker exec -i $CONTAINER_NAME bash %1 ").arg(fileName);
    e = runScript(credentials,
        replaceVars(runner, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);

    QString remover = QString("sudo docker exec -i $CONTAINER_NAME rm %1 ").arg(fileName);
    runScript(credentials,
        replaceVars(remover, genVarsForScript(credentials, container)));

    return e;
}

ErrorCode ServerController::uploadTextFileToContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &file, const QString &path,
    QSsh::SftpOverwriteMode overwriteMode)
{
    ErrorCode e = ErrorCode::NoError;
    QString tmpFileName = QString("/tmp/%1.tmp").arg(Utils::getRandomString(16));
    e = uploadFileToHost(credentials, file.toUtf8(), tmpFileName);
    if (e) return e;

    QString stdOut;
    auto cbReadStd = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    // mkdir
    QString mkdir = QString("sudo docker exec -i $CONTAINER_NAME mkdir -p  \"$(dirname %1)\"")
            .arg(path);

    e = runScript(credentials,
        replaceVars(mkdir, genVarsForScript(credentials, container)));
    if (e) return e;


    if (overwriteMode == QSsh::SftpOverwriteMode::SftpOverwriteExisting) {
        e = runScript(credentials,
            replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(path),
                genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

        if (e) return e;
    }
    else if (overwriteMode == QSsh::SftpOverwriteMode::SftpAppendToExisting) {
        e = runScript(credentials,
            replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(tmpFileName),
                genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

        if (e) return e;

        e = runScript(credentials,
            replaceVars(QString("sudo docker exec -i $CONTAINER_NAME sh -c \"cat %1 >> %2\"").arg(tmpFileName).arg(path),
                genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

        if (e) return e;
    }
    else return ErrorCode::NotImplementedError;


    if (stdOut.contains("Error: No such container:")) {
        return ErrorCode::ServerContainerMissingError;
    }

    runScript(credentials,
        replaceVars(QString("sudo shred %1").arg(tmpFileName),
              genVarsForScript(credentials, container)));

    runScript(credentials,
            replaceVars(QString("sudo rm %1").arg(tmpFileName),
                genVarsForScript(credentials, container)));

    return e;
}

QByteArray ServerController::getTextFileFromContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode)
{
    if (errorCode) *errorCode = ErrorCode::NoError;

    QString script = QString("sudo docker exec -i %1 sh -c \"xxd -p \'%2\'\"").
            arg(ContainerProps::containerToString(container)).arg(path);

    qDebug().noquote() << "Copy file from container\n" << script;

    QSharedPointer<SshConnection> client = connectToHost(sshParams(credentials));
    if (client->state() != SshConnection::State::Connected) {
        if (errorCode) *errorCode = fromSshConnectionErrorCode(client->errorState());
        return {};
    }

    QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(script.toUtf8());
    if (!proc) {
        qCritical() << "Failed to create SshRemoteProcess, breaking.";
        if (errorCode) *errorCode = ErrorCode::SshRemoteProcessCreationError;
        return {};
    }

    QEventLoop wait;
    int exitStatus = 0;

    QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
        exitStatus = status;
        wait.quit();
    });

    QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [&](){
        qDebug() << "ServerController::getTextFileFromContainer proc started";
        exitStatus = -1;
    });

    proc->start();
    wait.exec();

//    if (exitStatus < 0) {
//        wait.exec();
//    }

    if (SshRemoteProcess::ExitStatus(exitStatus) != QSsh::SshRemoteProcess::ExitStatus::NormalExit) {
        if (errorCode) *errorCode = fromSshProcessExitStatus(exitStatus);
    }

    if (errorCode) *errorCode = ErrorCode::NoError;
    return QByteArray::fromHex(proc->readAllStandardOutput());
}

ErrorCode ServerController::checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials)
{
    QString caCert = ServerController::getTextFileFromContainer(container,
        credentials, protocols::openvpn::caCertPath);
    QString taKey = ServerController::getTextFileFromContainer(container,
        credentials, protocols::openvpn::taKeyPath);

    if (!caCert.isEmpty() && !taKey.isEmpty()) {
        return ErrorCode::NoError;
    }
    else {
        return ErrorCode::ServerCheckFailed;
    }
}

ErrorCode ServerController::uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath,
    QSsh::SftpOverwriteMode overwriteMode)
{
    QSharedPointer<SshConnection> client = connectToHost(sshParams(credentials));
    if (client->state() != SshConnection::State::Connected) {
        return fromSshConnectionErrorCode(client->errorState());
    }

    bool err = false;

    QEventLoop wait;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(3000);

    QSharedPointer<SftpChannel> sftp = client->createSftpChannel();
    sftp->initialize();

    QObject::connect(sftp.data(), &SftpChannel::initialized, &wait, [&](){
        timer.stop();
        wait.quit();
    });
    QObject::connect(&timer, &QTimer::timeout, &wait, [&](){
        err= true;
        wait.quit();
    });

    wait.exec();

    if (!sftp) {
        qCritical() << "Failed to create SftpChannel, breaking.";
        return ErrorCode::SshRemoteProcessCreationError;
    }

    QTemporaryFile localFile;
    localFile.open();
    localFile.write(data);
    localFile.close();

    auto job = sftp->uploadFile(localFile.fileName(), remotePath, QSsh::SftpOverwriteMode::SftpOverwriteExisting);
    QObject::connect(sftp.data(), &SftpChannel::finished, &wait, [&](QSsh::SftpJobId j, const SftpError errorType = SftpError::NoError, const QString &error = QString()){
        if (job == j) {
            qDebug() << "Sftp finished with status" << error;
            wait.quit();
        }
    });

    QObject::connect(sftp.data(), &SftpChannel::channelError, &wait, [&](const QString &reason){
        qDebug() << "Sftp finished with error" << reason;
        err= true;
        wait.quit();
    });

    wait.exec();

    if (err) {
        return ErrorCode::SshSftpError;
    }
    else {
        return ErrorCode::NoError;
    }
}

ErrorCode ServerController::fromSshConnectionErrorCode(SshError error)
{
    switch (error) {
    case(SshNoError): return ErrorCode::NoError;
    case(QSsh::SshSocketError): return ErrorCode::SshSocketError;
    case(QSsh::SshTimeoutError): return ErrorCode::SshTimeoutError;
    case(QSsh::SshProtocolError): return ErrorCode::SshProtocolError;
    case(QSsh::SshHostKeyError): return ErrorCode::SshHostKeyError;
    case(QSsh::SshKeyFileError): return ErrorCode::SshKeyFileError;
    case(QSsh::SshAuthenticationError): return ErrorCode::SshAuthenticationError;
    case(QSsh::SshClosedByServerError): return ErrorCode::SshClosedByServerError;
    case(QSsh::SshInternalError): return ErrorCode::SshInternalError;
    default: return ErrorCode::SshInternalError;
    }
}

ErrorCode ServerController::fromSshProcessExitStatus(int exitStatus)
{
    qDebug() << exitStatus;
    switch (SshRemoteProcess::ExitStatus(exitStatus)) {
    case(SshRemoteProcess::ExitStatus::NormalExit): return ErrorCode::NoError;
    case(SshRemoteProcess::ExitStatus::FailedToStart): return ErrorCode::FailedToStartRemoteProcessError;
    case(SshRemoteProcess::ExitStatus::CrashExit): return ErrorCode::RemoteProcessCrashError;
    default: return ErrorCode::SshInternalError;
    }
}

SshConnectionParameters ServerController::sshParams(const ServerCredentials &credentials)
{
    QSsh::SshConnectionParameters sshParams;
    if (credentials.password.contains("BEGIN") && credentials.password.contains("PRIVATE KEY")) {
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
        sshParams.privateKeyFile = credentials.password;
    }
    else {
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;
        sshParams.setPassword(credentials.password);
    }
    sshParams.setHost(credentials.hostName);
    sshParams.setUserName(credentials.userName);
    sshParams.timeout = 10;
    sshParams.setPort(credentials.port);
    sshParams.hostKeyCheckingMode = QSsh::SshHostKeyCheckingMode::SshHostKeyCheckingNone;
    sshParams.options = SshIgnoreDefaultProxy;

    return sshParams;
}

ErrorCode ServerController::removeAllContainers(const ServerCredentials &credentials)
{
    return runScript(credentials,
        amnezia::scriptData(SharedScriptType::remove_all_containers));
}

ErrorCode ServerController::removeContainer(const ServerCredentials &credentials, DockerContainer container)
{
    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::remove_container),
            genVarsForScript(credentials, container)));
}

ErrorCode ServerController::setupContainer(const ServerCredentials &credentials, DockerContainer container,
                                           QJsonObject &config, bool isUpdate)
{
    qDebug().noquote() << "ServerController::setupContainer" << ContainerProps::containerToString(container);
    //qDebug().noquote() << QJsonDocument(config).toJson();
    ErrorCode e = ErrorCode::NoError;

    if (!isUpdate) {
        e = isServerPortBusy(credentials, container, config);
        if (e) return e;
    }

    e = installDockerWorker(credentials, container);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer installDockerWorker finished";

    e = prepareHostWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer prepareHostWorker finished";

    removeContainer(credentials, container);
    qDebug().noquote() << "ServerController::setupContainer removeContainer finished";

    e = buildContainerWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer buildContainerWorker finished";

    e = runContainerWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer runContainerWorker finished";

    e = configureContainerWorker(credentials, container, config);
    if (e) return e;
    qDebug().noquote() << "ServerController::setupContainer configureContainerWorker finished";

    setupServerFirewall(credentials);
    qDebug().noquote() << "ServerController::setupContainer setupServerFirewall finished";

    return startupContainerWorker(credentials, container, config);
}

ErrorCode ServerController::updateContainer(const ServerCredentials &credentials, DockerContainer container,
    const QJsonObject &oldConfig, QJsonObject &newConfig)
{
    bool reinstallRequired = isReinstallContainerRequired(container, oldConfig, newConfig);
    qDebug() << "ServerController::updateContainer for container" << container << "reinstall required is" << reinstallRequired;

    if (reinstallRequired) {
        return setupContainer(credentials, container, newConfig, true);
    }
    else {
        ErrorCode e = configureContainerWorker(credentials, container, newConfig);
        if (e) return e;

        return startupContainerWorker(credentials, container, newConfig);
    }
}

QJsonObject ServerController::createContainerInitialConfig(DockerContainer container, int port, TransportProto tp)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    QJsonObject config {
        { config_key::container, ContainerProps::containerToString(container) }
    };

    QJsonObject protoConfig;
    protoConfig.insert(config_key::port, QString::number(port));
    protoConfig.insert(config_key::transport_proto, ProtocolProps::transportProtoToString(tp, mainProto));


    if (container == DockerContainer::Sftp) {
        protoConfig.insert(config_key::userName, protocols::sftp::defaultUserName);
        protoConfig.insert(config_key::password, Utils::getRandomString(10));
    }

    config.insert(ProtocolProps::protoToString(mainProto), protoConfig);

    return config;
}

bool ServerController::isReinstallContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
{
    Proto mainProto = ContainerProps::defaultProtocol(container);

    const QJsonObject &oldProtoConfig = oldConfig.value(ProtocolProps::protoToString(mainProto)).toObject();
    const QJsonObject &newProtoConfig = newConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

    if (container == DockerContainer::OpenVpn) {
        if (oldProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) !=
            newProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto))
                return true;

        if (oldProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort))
                return true;
    }

    if (container == DockerContainer::Cloak) {
        if (oldProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort))
                return true;
    }

    if (container == DockerContainer::ShadowSocks) {
         if (oldProtoConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort))
                return true;
    }

    return false;
}

ErrorCode ServerController::installDockerWorker(const ServerCredentials &credentials, DockerContainer container)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";

        if (data.contains("Automatically restart Docker daemon?")) {
            proc->write("yes\n");
        }
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    QFutureWatcher<ErrorCode> watcher;

    QFuture<ErrorCode> future = QtConcurrent::run([this, &stdOut, &cbReadStdOut, &cbReadStdErr, &credentials]() {
        do {
            if (m_cancelInstallation) {
                return ErrorCode::ServerCancelInstallation;
            }
            stdOut.clear();
            runScript(credentials,
                      replaceVars(amnezia::scriptData(SharedScriptType::check_server_is_busy),
                                  genVarsForScript(credentials)), cbReadStdOut, cbReadStdErr);
            if (!stdOut.isEmpty() || stdOut.contains("Unable to acquire the dpkg frontend lock")) {
                emit serverIsBusy(true);
                QThread::msleep(1000);
            }
        } while (!stdOut.isEmpty());
        return ErrorCode::NoError;
    });

    QEventLoop wait;
    QObject::connect(&watcher, &QFutureWatcher<ErrorCode>::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    wait.exec();

    m_cancelInstallation = false;
    emit serverIsBusy(false);

    if (future.result() != ErrorCode::NoError) {
        return future.result();
    }

    ErrorCode error = runScript(credentials,
                                replaceVars(amnezia::scriptData(SharedScriptType::install_docker),
                                            genVarsForScript(credentials)), cbReadStdOut, cbReadStdErr);

    if (stdOut.contains("command not found")) return ErrorCode::ServerDockerFailedError;

    return error;
}

ErrorCode ServerController::prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    // create folder on host
    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::prepare_host),
            genVarsForScript(credentials, container)));
}

ErrorCode ServerController::buildContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    ErrorCode e = uploadFileToHost(credentials, amnezia::scriptData(ProtocolScriptType::dockerfile, container).toUtf8(),
        amnezia::server::getDockerfileFolder(container) + "/Dockerfile");

    if (e) return e;

//    QString stdOut;
//    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };
//    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };

    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::build_container),
                    genVarsForScript(credentials, container, config)));
}

ErrorCode ServerController::runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(credentials,
        replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, container),
            genVarsForScript(credentials, container, config)),
                cbReadStdOut, cbReadStdErr);

    if (stdOut.contains("address already in use")) return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container")) return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("invalid publish")) return ErrorCode::ServerDockerFailedError;

    return e;
}

ErrorCode ServerController::configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };


    ErrorCode e = runContainerScript(credentials, container,
        replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, container),
            genVarsForScript(credentials, container, config)),
                cbReadStdOut, cbReadStdErr);


    m_configurator->updateContainerConfigAfterInstallation(container, config, stdOut);

    return e;
}

ErrorCode ServerController::startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString script = amnezia::scriptData(ProtocolScriptType::container_startup, container);

    if (script.isEmpty()) {
        return ErrorCode::NoError;
    }

    ErrorCode e = uploadTextFileToContainer(container, credentials,
        replaceVars(script, genVarsForScript(credentials, container, config)),
            "/opt/amnezia/start.sh");
    if (e) return e;

    return runScript(credentials,
        replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && /opt/amnezia/start.sh\"",
            genVarsForScript(credentials, container, config)));
}

ServerController::Vars ServerController::genVarsForScript(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    const QJsonObject &openvpnConfig = config.value(ProtocolProps::protoToString(Proto::OpenVpn)).toObject();
    const QJsonObject &cloakConfig = config.value(ProtocolProps::protoToString(Proto::Cloak)).toObject();
    const QJsonObject &ssConfig = config.value(ProtocolProps::protoToString(Proto::ShadowSocks)).toObject();
    const QJsonObject &wireguarConfig = config.value(ProtocolProps::protoToString(Proto::WireGuard)).toObject();
    const QJsonObject &sftpConfig = config.value(ProtocolProps::protoToString(Proto::Sftp)).toObject();
    //

    Vars vars;

    vars.append({{"$REMOTE_HOST", credentials.hostName}});

    // OpenVPN vars
    vars.append({{"$OPENVPN_SUBNET_IP", openvpnConfig.value(config_key::subnet_address).toString(protocols::openvpn::defaultSubnetAddress) }});
    vars.append({{"$OPENVPN_SUBNET_CIDR", openvpnConfig.value(config_key::subnet_cidr).toString(protocols::openvpn::defaultSubnetCidr) }});
    vars.append({{"$OPENVPN_SUBNET_MASK", openvpnConfig.value(config_key::subnet_mask).toString(protocols::openvpn::defaultSubnetMask) }});

    vars.append({{"$OPENVPN_PORT", openvpnConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) }});
    vars.append({{"$OPENVPN_TRANSPORT_PROTO", openvpnConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) }});

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    vars.append({{"$OPENVPN_NCP_DISABLE",  isNcpDisabled ? protocols::openvpn::ncpDisableString : "" }});

    vars.append({{"$OPENVPN_CIPHER", openvpnConfig.value(config_key::cipher).toString(protocols::openvpn::defaultCipher) }});
    vars.append({{"$OPENVPN_HASH", openvpnConfig.value(config_key::hash).toString(protocols::openvpn::defaultHash) }});

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    vars.append({{"$OPENVPN_TLS_AUTH", isTlsAuth ? protocols::openvpn::tlsAuthString : "" }});
    if (!isTlsAuth) {
        // erase $OPENVPN_TA_KEY, so it will not set in OpenVpnConfigurator::genOpenVpnConfig
        vars.append({{"$OPENVPN_TA_KEY", "" }});
    }

    vars.append({{"$OPENVPN_ADDITIONAL_CLIENT_CONFIG", openvpnConfig.value(config_key::additional_client_config).
                  toString(protocols::openvpn::defaultAdditionalClientConfig) }});
    vars.append({{"$OPENVPN_ADDITIONAL_SERVER_CONFIG", openvpnConfig.value(config_key::additional_server_config).
                  toString(protocols::openvpn::defaultAdditionalServerConfig) }});

    // ShadowSocks vars
    vars.append({{"$SHADOWSOCKS_SERVER_PORT", ssConfig.value(config_key::port).toString(protocols::shadowsocks::defaultPort) }});
    vars.append({{"$SHADOWSOCKS_LOCAL_PORT", ssConfig.value(config_key::local_port).toString(protocols::shadowsocks::defaultLocalProxyPort) }});
    vars.append({{"$SHADOWSOCKS_CIPHER", ssConfig.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher) }});

    vars.append({{"$CONTAINER_NAME", ContainerProps::containerToString(container)}});
    vars.append({{"$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(container)}});

    // Cloak vars
    vars.append({{"$CLOAK_SERVER_PORT", cloakConfig.value(config_key::port).toString(protocols::cloak::defaultPort) }});
    vars.append({{"$FAKE_WEB_SITE_ADDRESS", cloakConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite) }});

    // Wireguard vars
    vars.append({{"$WIREGUARD_SUBNET_IP", wireguarConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress) }});
    vars.append({{"$WIREGUARD_SUBNET_CIDR", wireguarConfig.value(config_key::subnet_cidr).toString(protocols::wireguard::defaultSubnetCidr) }});
    vars.append({{"$WIREGUARD_SUBNET_MASK", wireguarConfig.value(config_key::subnet_mask).toString(protocols::wireguard::defaultSubnetMask) }});

    vars.append({{"$WIREGUARD_SERVER_PORT", wireguarConfig.value(config_key::port).toString(protocols::wireguard::defaultPort) }});

    // IPsec vars
    vars.append({{"$IPSEC_VPN_L2TP_NET", "192.168.42.0/24"}});
    vars.append({{"$IPSEC_VPN_L2TP_POOL", "192.168.42.10-192.168.42.250"}});
    vars.append({{"$IPSEC_VPN_L2TP_LOCAL", "192.168.42.1"}});

    vars.append({{"$IPSEC_VPN_XAUTH_NET", "192.168.43.0/24"}});
    vars.append({{"$IPSEC_VPN_XAUTH_POOL", "192.168.43.10-192.168.43.250"}});

    vars.append({{"$IPSEC_VPN_SHA2_TRUNCBUG", "yes"}});

    vars.append({{"$IPSEC_VPN_VPN_ANDROID_MTU_FIX", "yes"}});
    vars.append({{"$IPSEC_VPN_DISABLE_IKEV2", "no"}});
    vars.append({{"$IPSEC_VPN_DISABLE_L2TP", "no"}});
    vars.append({{"$IPSEC_VPN_DISABLE_XAUTH", "no"}});

    vars.append({{"$IPSEC_VPN_C2C_TRAFFIC", "no"}});

    vars.append({{"$PRIMARY_SERVER_DNS", m_settings->primaryDns()}});
    vars.append({{"$SECONDARY_SERVER_DNS", m_settings->secondaryDns()}});


    // Sftp vars
    vars.append({{"$SFTP_PORT", sftpConfig.value(config_key::port).toString(QString::number(ProtocolProps::defaultPort(Proto::Sftp))) }});
    vars.append({{"$SFTP_USER", sftpConfig.value(config_key::userName).toString() }});
    vars.append({{"$SFTP_PASSWORD", sftpConfig.value(config_key::password).toString() }});


    QString serverIp = Utils::getIPAddress(credentials.hostName);
    if (!serverIp.isEmpty()) {
        vars.append({{"$SERVER_IP_ADDRESS", serverIp}});
    }
    else {
        qWarning() << "ServerController::genVarsForScript unable to resolve address for credentials.hostName";
    }

    return vars;
}

QString ServerController::checkSshConnection(const ServerCredentials &credentials, ErrorCode *errorCode)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(credentials,
        amnezia::scriptData(SharedScriptType::check_connection), cbReadStdOut, cbReadStdErr);

    if (errorCode) *errorCode = e;

    return stdOut;
}

QSharedPointer<SshConnection> ServerController::connectToHost(const SshConnectionParameters &sshParams)
{
    QSharedPointer<SshConnection> client(new SshConnection(sshParams));
    if (!client.get()) return nullptr;

    QEventLoop waitssh;
    QObject::connect(client.get(), &SshConnection::connected, &waitssh, [&]() {
        qDebug() << "Server connected by ssh";
        waitssh.quit();
    });

    QObject::connect(client.get(), &SshConnection::disconnected, &waitssh, [&]() {
        qDebug() << "Server disconnected by ssh";
        waitssh.quit();
    });

    QObject::connect(client.get(), &SshConnection::error, &waitssh, [&](QSsh::SshError error) {
        qCritical() << "Ssh error:" << error << client->errorString();
        waitssh.quit();
    });


    //    QObject::connect(client, &SshConnection::dataAvailable, [&](const QString &message) {
    //        qCritical() << "Ssh message:" << message;
    //    });

    //qDebug() << "Connection state" << client->state();

    if (client->state() == SshConnection::State::Unconnected) {
        client->connectToHost();
        waitssh.exec();
    }


    //    QObject::connect(&client, &SshClient::sshDataReceived, [&](){
    //        qDebug().noquote() << "Data received";
    //    });


    //    if(client.sshState() != SshClient::SshState::Ready) {
    //        qCritical() << "Can't connect to server";
    //        return false;
    //    }
    //    else {
    //        qDebug() << "SSh connection established";
    //    }


    //    QObject::connect(proc, &SshProcess::finished, &wait, &QEventLoop::quit);
    //    QObject::connect(proc, &SshProcess::failed, &wait, &QEventLoop::quit);

    return client;
}

void ServerController::setCancelInstallation(const bool cancel)
{
    m_cancelInstallation = cancel;
}

ErrorCode ServerController::setupServerFirewall(const ServerCredentials &credentials)
{
    return runScript(credentials,
        replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall),
            genVarsForScript(credentials)));
}

QString ServerController::replaceVars(const QString &script, const Vars &vars)
{
    QString s = script;
    for (const QPair<QString, QString> &var : vars) {
        //qDebug() << "Replacing" << var.first << var.second;
        s.replace(var.first, var.second);
    }
    //qDebug().noquote() << script;
    return s;
}

ErrorCode ServerController::isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    if (container == DockerContainer::Dns) {
        return ErrorCode::NoError;
    }

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    const Proto protocol = ContainerProps::defaultProtocol(container);
    const QString containerString = ProtocolProps::protoToString(protocol);
    const QJsonObject containerConfig = config.value(containerString).toObject();

    QStringList fixedPorts = ContainerProps::fixedPortsForContainer(container);

    QString defaultPort("%1");
    QString port = containerConfig.value(config_key::port).toString(defaultPort.arg(ProtocolProps::defaultPort(protocol)));
    QString defaultTransportProto = ProtocolProps::transportProtoToString(ProtocolProps::defaultTransportProto(protocol), protocol);
    QString transportProto = containerConfig.value(config_key::transport_proto).toString(defaultTransportProto);

    QString script = QString("sudo lsof -i -P -n | grep -E ':%1 ").arg(port);
    for (auto &port : fixedPorts) {
        script = script.append("|:%1").arg(port);
    }
    script = script.append("' | grep -i %1").arg(transportProto);
    ErrorCode errorCode = runScript(credentials,
              replaceVars(script, genVarsForScript(credentials, container)), cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (!stdOut.isEmpty()) {
        return ErrorCode::ServerPortAlreadyAllocatedError;
    }
    return ErrorCode::NoError;
}

ErrorCode ServerController::getAlreadyInstalledContainers(const ServerCredentials &credentials, QMap<DockerContainer, QJsonObject> &installedContainers)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    QString script = QString("sudo docker ps --format '{{.Names}} {{.Ports}}'");

    ErrorCode errorCode = runScript(credentials, script, cbReadStdOut, cbReadStdErr);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    auto containersInfo = stdOut.split("\n");
    for (auto &containerInfo : containersInfo) {
        if (containerInfo.isEmpty()) {
            continue;
        }
        const static QRegularExpression containerAndPortRegExp("(amnezia[-a-z]*).*?:([0-9]*)->[0-9]*/(udp|tcp).*");
        QRegularExpressionMatch containerAndPortMatch = containerAndPortRegExp.match(containerInfo);
        if (containerAndPortMatch.hasMatch()) {
            QString name = containerAndPortMatch.captured(1);
            QString port = containerAndPortMatch.captured(2);
            QString transportProto = containerAndPortMatch.captured(3);
            DockerContainer container = ContainerProps::containerFromString(name);
            Proto mainProto = ContainerProps::defaultProtocol(container);
            QJsonObject config {
                { config_key::container, name },
                { ProtocolProps::protoToString(mainProto), QJsonObject {
                    { config_key::port, port },
                    { config_key::transport_proto, transportProto }}
                }
            };
            installedContainers.insert(container, config);
        }
    }

    return ErrorCode::NoError;
}
