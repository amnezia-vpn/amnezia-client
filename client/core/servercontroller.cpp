#include "servercontroller.h"

#include <QCryptographicHash>
#include <QFile>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QApplication>
#include <QTemporaryFile>

#include "sftpchannel.h"
#include "sshconnectionmanager.h"

#include "protocols/protocols_defs.h"
#include "server_defs.h"
#include "scripts_registry.h"
#include "utils.h"


using namespace QSsh;

ErrorCode ServerController::runScript(const SshConnectionParameters &sshParams, QString script,
    const std::function<void(const QString &, QSharedPointer<SshRemoteProcess>)> &cbReadStdOut,
    const std::function<void(const QString &, QSharedPointer<SshRemoteProcess>)> &cbReadStdErr)
{
    SshConnection *client = connectToHost(sshParams);
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
    const QStringList &lines = script.split("\n", QString::SkipEmptyParts);
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
                qDebug().noquote() << "stdout" << s;
            }
            if (cbReadStdOut) cbReadStdOut(s, proc);
        });

        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, &wait, [proc, cbReadStdErr](){
            QString s = proc->readAllStandardError();
            if (s != "." && !s.isEmpty()) {
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

ErrorCode ServerController::uploadTextFileToContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &file, const QString &path)
{
    ErrorCode e = ErrorCode::NoError;
    QString tmpFileName = QString("/tmp/%1.tmp").arg(Utils::getRandomString(16));
    e = uploadFileToHost(credentials, file.toUtf8(), tmpFileName);
    if (e) return e;

    QString stdOut;
    auto cbReadStd = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> ) {
        stdOut += data + "\n";
    };

    e = runScript(sshParams(credentials),
        replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(path),
            genVarsForScript(credentials, container)), cbReadStd, cbReadStd);

    if (e) return e;
    if (stdOut.contains("Error: No such container:")) {
        return ErrorCode::ServerContainerMissingError;
    }

    runScript(sshParams(credentials),
        replaceVars(QString("sudo shred %1").arg(tmpFileName),
              genVarsForScript(credentials, container)));

    runScript(sshParams(credentials),
            replaceVars(QString("sudo rm %1").arg(tmpFileName),
                genVarsForScript(credentials, container)));

    return e;
}

QString ServerController::getTextFileFromContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode)
{
    if (errorCode) *errorCode = ErrorCode::NoError;

    QString script = QString("sudo docker exec -i %1 sh -c \"cat \'%2\'\"").
            arg(amnezia::containerToString(container)).arg(path);

    qDebug().noquote() << "Copy file from container\n" << script;

    SshConnection *client = connectToHost(sshParams(credentials));
    if (client->state() != SshConnection::State::Connected) {
        if (errorCode) *errorCode = fromSshConnectionErrorCode(client->errorState());
        return QString();
    }

    QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(script.toUtf8());
    if (!proc) {
        qCritical() << "Failed to create SshRemoteProcess, breaking.";
        if (errorCode) *errorCode = ErrorCode::SshRemoteProcessCreationError;
        return QString();
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

    return proc->readAllStandardOutput();
}

ErrorCode ServerController::checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials)
{
    QString caCert = ServerController::getTextFileFromContainer(container,
        credentials, amnezia::protocols::openvpn::caCertPath);
    QString taKey = ServerController::getTextFileFromContainer(container,
        credentials, amnezia::protocols::openvpn::taKeyPath);

    if (!caCert.isEmpty() && !taKey.isEmpty()) {
        return ErrorCode::NoError;
    }
    else {
        return ErrorCode::ServerCheckFailed;
    }
}

ErrorCode ServerController::uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath)
{
    SshConnection *client = connectToHost(sshParams(credentials));
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
    QObject::connect(sftp.data(), &SftpChannel::finished, &wait, [&](QSsh::SftpJobId j, const QString &error){
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
        sshParams.password = credentials.password;
    }
    sshParams.host = credentials.hostName;
    sshParams.userName = credentials.userName;
    sshParams.timeout = 10;
    sshParams.port = credentials.port;
    sshParams.hostKeyCheckingMode = QSsh::SshHostKeyCheckingMode::SshHostKeyCheckingNone;
    sshParams.options = SshIgnoreDefaultProxy;

    return sshParams;
}

ErrorCode ServerController::removeAllContainers(const ServerCredentials &credentials)
{
    return runScript(sshParams(credentials),
        amnezia::scriptData(SharedScriptType::remove_all_containers));
}

ErrorCode ServerController::removeContainer(const ServerCredentials &credentials, DockerContainer container)
{
    return runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(SharedScriptType::remove_container),
            genVarsForScript(credentials, container)));
}

ErrorCode ServerController::setupContainer(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    qDebug().noquote() << "ServerController::setupContainer" << containerToString(container);
    qDebug().noquote() << QJsonDocument(config).toJson();
    ErrorCode e = ErrorCode::NoError;

    e = installDockerWorker(credentials, container);
    if (e) return e;

    e = prepareHostWorker(credentials, container, config);
    if (e) return e;

    removeContainer(credentials, container);

    e = buildContainerWorker(credentials, container, config);
    if (e) return e;

    e = runContainerWorker(credentials, container, config);
    if (e) return e;

    e = configureContainerWorker(credentials, container, config);
    if (e) return e;

    return startupContainerWorker(credentials, container, config);
}

ErrorCode ServerController::updateContainer(const ServerCredentials &credentials, DockerContainer container,
    const QJsonObject &oldConfig, const QJsonObject &newConfig)
{
    bool reinstallRequred = isReinstallContainerRequred(container, oldConfig, newConfig);
    qDebug() << "ServerController::updateContainer for container" << container << "reinstall required is" << reinstallRequred;

    if (reinstallRequred) {
        return setupContainer(credentials, container, newConfig);
    }
    else {
        ErrorCode e = configureContainerWorker(credentials, container, newConfig);
        if (e) return e;

        return startupContainerWorker(credentials, container, newConfig);
    }
}

bool ServerController::isReinstallContainerRequred(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
{
    if (container == DockerContainer::OpenVpn) {
        const QJsonObject &oldProtoConfig = oldConfig[config_key::openvpn].toObject();
        const QJsonObject &newProtoConfig = newConfig[config_key::openvpn].toObject();

        if (oldProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto) !=
            newProtoConfig.value(config_key::transport_proto).toString(protocols::openvpn::defaultTransportProto))
                return true;

        if (oldProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::openvpn::defaultPort))
                return true;
    }

    if (container == DockerContainer::OpenVpnOverCloak) {
        const QJsonObject &oldProtoConfig = oldConfig[config_key::cloak].toObject();
        const QJsonObject &newProtoConfig = newConfig[config_key::cloak].toObject();

        if (oldProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort) !=
            newProtoConfig.value(config_key::port).toString(protocols::cloak::defaultPort))
                return true;
    }

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        const QJsonObject &oldProtoConfig = oldConfig[config_key::shadowsocks].toObject();
        const QJsonObject &newProtoConfig = newConfig[config_key::shadowsocks].toObject();

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

    return runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(SharedScriptType::install_docker),
            genVarsForScript(credentials, container)),
                cbReadStdOut, cbReadStdErr);
}

ErrorCode ServerController::prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    // create folder on host
    return runScript(sshParams(credentials),
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

    return runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(SharedScriptType::build_container),
                    genVarsForScript(credentials, container, config)));
}

ErrorCode ServerController::runContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(ProtocolScriptType::run_container, container),
            genVarsForScript(credentials, container, config)),
                cbReadStdOut, cbReadStdErr);

    if (stdOut.contains("address already in use")) return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("is already in use by container")) return ErrorCode::ServerPortAlreadyAllocatedError;

    return e;
}

ErrorCode ServerController::configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    return runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, container),
            genVarsForScript(credentials, container, config)));
}

ErrorCode ServerController::startupContainerWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    ErrorCode e = uploadTextFileToContainer(container, credentials,
        replaceVars(amnezia::scriptData(ProtocolScriptType::container_startup, container),
             genVarsForScript(credentials, container, config)),
        "/opt/amnezia/start.sh");
    if (e) return e;

    return runScript(sshParams(credentials),
        replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && /opt/amnezia/start.sh\"",
            genVarsForScript(credentials, container, config)));
}

ServerController::Vars ServerController::   genVarsForScript(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config)
{
    const QJsonObject &openvpnConfig = config.value(config_key::openvpn).toObject();
    const QJsonObject &cloakConfig = config.value(config_key::cloak).toObject();
    const QJsonObject &ssConfig = config.value(config_key::shadowsocks).toObject();
    //

    Vars vars;

    vars.append({{"$REMOTE_HOST", credentials.hostName}});

    // OpenVPN vars
    vars.append({{"$VPN_SUBNET_IP", openvpnConfig.value(config_key::subnet_address).toString(amnezia::protocols::vpnDefaultSubnetAddress) }});
    vars.append({{"$VPN_SUBNET_MASK_VAL", openvpnConfig.value(config_key::subnet_mask_val).toString(amnezia::protocols::vpnDefaultSubnetMaskVal) }});
    vars.append({{"$VPN_SUBNET_MASK", openvpnConfig.value(config_key::subnet_mask).toString(amnezia::protocols::vpnDefaultSubnetMask) }});

    vars.append({{"$OPENVPN_PORT", openvpnConfig.value(config_key::port).toString(amnezia::protocols::openvpn::defaultPort) }});
    vars.append({{"$OPENVPN_TRANSPORT_PROTO", openvpnConfig.value(config_key::transport_proto).toString(amnezia::protocols::openvpn::defaultTransportProto) }});

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(amnezia::protocols::openvpn::defaultNcpDisable);
    vars.append({{"$OPENVPN_NCP_DISABLE",  isNcpDisabled ? protocols::openvpn::ncpDisableString : "" }});

    vars.append({{"$OPENVPN_CIPHER", openvpnConfig.value(config_key::cipher).toString(amnezia::protocols::openvpn::defaultCipher) }});
    vars.append({{"$OPENVPN_HASH", openvpnConfig.value(config_key::hash).toString(amnezia::protocols::openvpn::defaultHash) }});

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(amnezia::protocols::openvpn::defaultTlsAuth);
    vars.append({{"$OPENVPN_TLS_AUTH", isTlsAuth ? protocols::openvpn::tlsAuthString : "" }});
    if (!isTlsAuth) {
        // erase $OPENVPN_TA_KEY, so it will not set in OpenVpnConfigurator::genOpenVpnConfig
        vars.append({{"$OPENVPN_TA_KEY", "" }});
    }

    // ShadowSocks vars
    vars.append({{"$SHADOWSOCKS_SERVER_PORT", ssConfig.value(config_key::port).toString(amnezia::protocols::shadowsocks::defaultPort) }});
    vars.append({{"$SHADOWSOCKS_LOCAL_PORT", ssConfig.value(config_key::local_port).toString(amnezia::protocols::shadowsocks::defaultLocalProxyPort) }});
    vars.append({{"$SHADOWSOCKS_CIPHER", ssConfig.value(config_key::cipher).toString(amnezia::protocols::shadowsocks::defaultCipher) }});

    vars.append({{"$CONTAINER_NAME", amnezia::containerToString(container)}});
    vars.append({{"$DOCKERFILE_FOLDER", "/opt/amnezia/" + amnezia::containerToString(container)}});

    // Cloak vars
    vars.append({{"$CLOAK_SERVER_PORT", cloakConfig.value(config_key::port).toString(protocols::cloak::defaultPort) }});
    vars.append({{"$FAKE_WEB_SITE_ADDRESS", cloakConfig.value(config_key::site).toString(protocols::cloak::defaultRedirSite) }});

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

    ErrorCode e = runScript(sshParams(credentials),
        amnezia::scriptData(SharedScriptType::check_connection), cbReadStdOut, cbReadStdErr);

    if (errorCode) *errorCode = e;

    return stdOut;
}

SshConnection *ServerController::connectToHost(const SshConnectionParameters &sshParams)
{
    SshConnection *client = acquireConnection(sshParams);
    if (!client) return nullptr;

    QEventLoop waitssh;
    QObject::connect(client, &SshConnection::connected, &waitssh, [&]() {
        qDebug() << "Server connected by ssh";
        waitssh.quit();
    });

    QObject::connect(client, &SshConnection::disconnected, &waitssh, [&]() {
        qDebug() << "Server disconnected by ssh";
        waitssh.quit();
    });

    QObject::connect(client, &SshConnection::error, &waitssh, [&](QSsh::SshError error) {
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

void ServerController::disconnectFromHost(const ServerCredentials &credentials)
{
    SshConnection *client = acquireConnection(sshParams(credentials));
    if (client) client->disconnectFromHost();
}

ErrorCode ServerController::setupServerFirewall(const ServerCredentials &credentials)
{
    return runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(SharedScriptType::setup_host_firewall),
            genVarsForScript(credentials, DockerContainer::OpenVpnOverCloak)));
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
