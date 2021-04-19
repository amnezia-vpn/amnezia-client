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
    if (client->state() != SshConnection::State::Connected) {
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

ErrorCode ServerController::installDocker(const ServerCredentials &credentials)
{
    // Setup openvpn part
    QString scriptData = amnezia::scriptData(SharedScriptType::install_docker);
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

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
        replaceVars(scriptData, genVarsForScript(credentials, DockerContainer::OpenVpnOverCloak)),
        cbReadStdOut, cbReadStdErr);
}

ErrorCode ServerController::uploadTextFileToContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &file, const QString &path)
{
    ErrorCode e;
    QString tmpFileName = QString("/tmp/%1.tmp").arg(Utils::getRandomString(16));
    uploadFileToHost(credentials, file.toUtf8(), tmpFileName);

    e = runScript(sshParams(credentials),
        replaceVars(QString("sudo docker cp %1 $CONTAINER_NAME:/%2").arg(tmpFileName).arg(path),
            genVarsForScript(credentials, container)));

    if (e) return e;

    runScript(sshParams(credentials),
        replaceVars(QString("sudo shred %1").arg(tmpFileName),
              genVarsForScript(credentials, container)));

    runScript(sshParams(credentials),
            replaceVars(QString("sudo rm %1").arg(tmpFileName),
                genVarsForScript(credentials, container)));

    return e;

    //    QString script = QString("sudo docker exec -i %1 sh -c \"echo \'%2\' > %3\"").
//            arg(amnezia::server::getContainerName(container)).arg(file).arg(path);

//    qDebug().noquote() << "uploadTextFileToContainer\n" << script;

//    SshConnection *client = connectToHost(sshParams(credentials));
//    if (client->state() != SshConnection::State::Connected) {
//        return fromSshConnectionErrorCode(client->errorState());
//    }

//    QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(script.toUtf8());

//    if (!proc) {
//        qCritical() << "Failed to create SshRemoteProcess, breaking.";
//        return ErrorCode::SshRemoteProcessCreationError;
//    }

//    QEventLoop wait;
//    int exitStatus = -1;

////    QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
////        qDebug() << "uploadTextFileToContainer started";
////    });

//    QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
//        //qDebug() << "Remote process exited with status" << status;
//        exitStatus = status;
//        wait.quit();
//    });

//    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, [proc](){
//        qDebug().noquote() << proc->readAllStandardOutput();
//    });

//    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, [proc](){
//        qDebug().noquote() << proc->readAllStandardError();
//    });

//    proc->start();

//    if (exitStatus < 0) {
//        wait.exec();
//    }

//    return fromSshProcessExitStatus(exitStatus);
}

QString ServerController::getTextFileFromContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode)
{
    if (errorCode) *errorCode = ErrorCode::NoError;

    QString script = QString("sudo docker exec -i %1 sh -c \"cat \'%2\'\"").
            arg(amnezia::server::getContainerName(container)).arg(path);

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
        credentials, amnezia::protocols::openvpn::caCertPath());
    QString taKey = ServerController::getTextFileFromContainer(container,
        credentials, amnezia::protocols::openvpn::taKeyPath());

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

ErrorCode ServerController::removeServer(const ServerCredentials &credentials, Protocol proto)
{
    QString scriptFileName;
    DockerContainer container;

    if (proto == Protocol::Any) {
        ErrorCode e = removeServer(credentials, Protocol::OpenVpn);
        if (e) {
            return e;
        }
        return removeServer(credentials, Protocol::ShadowSocksOverOpenVpn);
    }
    else if (proto == Protocol::OpenVpn) {
        scriptFileName = ":/server_scripts/remove_container.sh";
        container = DockerContainer::OpenVpn;
    }
    else if (proto == Protocol::ShadowSocksOverOpenVpn) {
        scriptFileName = ":/server_scripts/remove_container.sh";
        container = DockerContainer::ShadowSocksOverOpenVpn;
    }
    else return ErrorCode::NotImplementedError;


    QString scriptData;

    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

    return runScript(sshParams(credentials), replaceVars(scriptData, genVarsForScript(credentials, container)));
}

ErrorCode ServerController::setupServer(const ServerCredentials &credentials, Protocol proto)
{
    ErrorCode e = runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(SharedScriptType::install_docker),
            genVarsForScript(credentials)));
    if (e) return e;

    if (proto == Protocol::OpenVpn) {
        return ErrorCode::NoError;
        //return setupOpenVpnServer(credentials);
    }
    else if (proto == Protocol::ShadowSocksOverOpenVpn) {
        return setupShadowSocksServer(credentials);
    }
    else if (proto == Protocol::Any) {
        //return ErrorCode::NotImplementedError;

        //setupOpenVpnServer(credentials);
        //return setupShadowSocksServer(credentials);
        return setupOpenVpnOverCloakServer(credentials);
    }

    return ErrorCode::NoError;
}

ErrorCode ServerController::setupOpenVpnServer(const ServerCredentials &credentials)
{
    return ErrorCode::NotImplementedError;

//    QString scriptData;
//    QString scriptFileName = ":/server_scripts/setup_openvpn_server.sh";
//    QFile file(scriptFileName);
//    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

//    scriptData = file.readAll();
//    if (scriptData.isEmpty()) return ErrorCode::InternalError;

//    QString stdOut;
//    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";

//        if (data.contains("Automatically restart Docker daemon?")) {
//            proc->write("yes\n");
//        }
//    };
//    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };

//    ErrorCode e = runScript(genVarsForScript(credentials, DockerContainer::OpenVpn), sshParams(credentials), scriptData, cbReadStdOut, cbReadStdErr);
//    if (e) return e;
//    QApplication::processEvents();

//    if (stdOut.contains("port is already allocated")) return ErrorCode::ServerPortAlreadyAllocatedError;
//    if (stdOut.contains("Error response from daemon")) return ErrorCode::ServerCheckFailed;

//    return checkOpenVpnServer(DockerContainer::OpenVpn, credentials);
}

ErrorCode ServerController::setupOpenVpnOverCloakServer(const ServerCredentials &credentials)
{
    ErrorCode e;
    DockerContainer container = DockerContainer::OpenVpnOverCloak;

    // create folder on host
    e = runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(SharedScriptType::prepare_host),
            genVarsForScript(credentials, container)));
    if (e) return e;

    uploadFileToHost(credentials, amnezia::scriptData(ProtocolScriptType::dockerfile, Protocol::OpenVpnOverCloak).toUtf8(),
        amnezia::server::getDockerfileFolder(container) + "/Dockerfile");


    // Setup openvpn part
    QString scriptData = amnezia::scriptData(SharedScriptType::build_container);
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

//    QString stdOut;
//    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };
//    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };

    e = runScript(sshParams(credentials),
        replaceVars(scriptData,
            genVarsForScript(credentials, container)));
    if (e) return e;


    runScript(sshParams(credentials),
        replaceVars(amnezia::scriptData(ProtocolScriptType::configure_container, Protocol::OpenVpnOverCloak),
            genVarsForScript(credentials, container)));
    if (e) return e;

    uploadTextFileToContainer(DockerContainer::OpenVpnOverCloak, credentials,
        replaceVars(amnezia::scriptData(ProtocolScriptType::container_startup, Protocol::OpenVpnOverCloak),
             genVarsForScript(credentials, container)),
        "/opt/amnezia/start.sh");

//    qDebug().noquote() << "AAAA"
// << amnezia::scriptData(ProtocolScriptType::container_startup, Protocol::OpenVpnOverCloak),
//            replaceVars("/opt/amnezia/start.sh",
//                genVarsForScript(credentials, container));

    runScript(sshParams(credentials),
        replaceVars("sudo docker exec -d $CONTAINER_NAME sh -c \"chmod a+x /opt/amnezia/start.sh && /opt/amnezia/start.sh\"",
            genVarsForScript(credentials, container)));
    if (e) return e;

    return e;
}

ErrorCode ServerController::setupShadowSocksServer(const ServerCredentials &credentials)
{
    return ErrorCode::NotImplementedError;
//    // Setup openvpn part
//    QString scriptData;
//    QString scriptFileName = ":/server_scripts/setup_shadowsocks_server.sh";
//    QFile file(scriptFileName);
//    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

//    scriptData = file.readAll();
//    if (scriptData.isEmpty()) return ErrorCode::InternalError;

//    QString stdOut;
//    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";

//        if (data.contains("Automatically restart Docker daemon?")) {
//            proc->write("yes\n");
//        }
//    };
//    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
//        stdOut += data + "\n";
//    };

//    ErrorCode e = runScript(genVarsForScript(credentials, DockerContainer::ShadowSocks), sshParams(credentials), scriptData, cbReadStdOut, cbReadStdErr);
//    if (e) return e;

//    // Create ss config
//    QJsonObject ssConfig;
//    ssConfig.insert("server", "0.0.0.0");
//    ssConfig.insert("server_port", amnezia::protocols::shadowsocks::ssRemotePort());
//    ssConfig.insert("local_port", amnezia::protocols::shadowsocks::ssContainerPort());
//    ssConfig.insert("password", QString(QCryptographicHash::hash(credentials.password.toUtf8(), QCryptographicHash::Sha256).toHex()));
//    ssConfig.insert("timeout", 60);
//    ssConfig.insert("method", amnezia::protocols::shadowsocks::ssEncryption());
//    QString configData = QJsonDocument(ssConfig).toJson();
//    QString sSConfigPath = "/opt/amneziavpn_data/ssConfig.json";

//    configData.replace("\"", "\\\"");
//    //qDebug().noquote() << configData;

//    uploadTextFileToContainer(DockerContainer::ShadowSocks, credentials, configData, sSConfigPath);

//    // Start ss
//    QString script = QString("sudo docker exec -d %1 sh -c \"ss-server -c %2\"").
//            arg(amnezia::server::getContainerName(DockerContainer::ShadowSocks)).arg(sSConfigPath);

//    e = runScript(genVarsForScript(credentials, DockerContainer::ShadowSocks), sshParams(credentials), script);
//    return e;
}

ServerController::Vars ServerController::genVarsForScript(const ServerCredentials &credentials, DockerContainer container)
{
    Vars vars;

    vars.append(qMakePair<QString, QString>("$VPN_SUBNET_IP", amnezia::server::vpnDefaultSubnetIp));
    vars.append(qMakePair<QString, QString>("$VPN_SUBNET_MASK_VAL", amnezia::server::vpnDefaultSubnetMaskVal));
    vars.append(qMakePair<QString, QString>("$VPN_SUBNET_MASK", amnezia::server::vpnDefaultSubnetMask));

    vars.append(qMakePair<QString, QString>("$CONTAINER_NAME", amnezia::server::getContainerName(container)));
    vars.append(qMakePair<QString, QString>("$DOCKERFILE_FOLDER", "/opt/amnezia/" + amnezia::server::getContainerName(container)));

    QString serverIp = Utils::getIPAddress(credentials.hostName);
    if (!serverIp.isEmpty()) {
        vars.append(qMakePair<QString, QString>("$SERVER_IP_ADDRESS", serverIp));
    }
    else {
        qWarning() << "ServerController::genVarsForScript unable to resolve address for credentials.hostName";
    }


    //
    if (container == DockerContainer::OpenVpn) {
        vars.append(qMakePair<QString, QString>("$SERVER_PORT", amnezia::protocols::openvpn::openvpnDefaultPort()));
    }
    else if (container == DockerContainer::OpenVpnOverCloak) {
        vars.append(qMakePair<QString, QString>("$SERVER_PORT", amnezia::protocols::cloak::ckDefaultPort()));
        vars.append(qMakePair<QString, QString>("$FAKE_WEB_SITE_ADDRESS", amnezia::protocols::cloak::ckDefaultRedirSite()));
    }
    else if (container == DockerContainer::ShadowSocksOverOpenVpn) {
        vars.append(qMakePair<QString, QString>("$SERVER_PORT", "6789"));
    }

    return vars;
}

SshConnection *ServerController::connectToHost(const SshConnectionParameters &sshParams)
{
    SshConnection *client = acquireConnection(sshParams);

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
        //qDebug() << "Replacing" << var << vars.value(var);
        s.replace(var.first, var.second);
    }
    return s;
}
