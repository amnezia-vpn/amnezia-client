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

#include "sshconnectionmanager.h"


using namespace QSsh;

QString ServerController::getContainerName(DockerContainer container)
{
    switch (container) {
    case(DockerContainer::OpenVpn): return "amnezia-openvpn";
    case(DockerContainer::ShadowSocks): return "amnezia-shadowsocks";
    default: return "";
    }
}

ErrorCode ServerController::runScript(DockerContainer container,
    const SshConnectionParameters &sshParams, QString script,
    const std::function<void(const QString &, QSharedPointer<SshRemoteProcess>)> &cbReadStdOut,
    const std::function<void(const QString &, QSharedPointer<SshRemoteProcess>)> &cbReadStdErr)
{
    SshConnection *client = connectToHost(sshParams);
    if (client->state() != SshConnection::State::Connected) {
        return fromSshConnectionErrorCode(client->errorState());
    }

    script.replace("\r", "");

    qDebug() << "Run script";

    const QStringList &lines = script.split("\n", QString::SkipEmptyParts);
    for (int i = 0; i < lines.count(); i++) {
        QString line = lines.at(i);
        line.replace("$CONTAINER_NAME", getContainerName(container));

        if (line.startsWith("#")) {
            continue;
        }

        qDebug().noquote() << "EXEC" << line;
        QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(line.toUtf8());

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
    const ServerCredentials &credentials, QString &file, const QString &path)
{
    QString script = QString("sudo docker exec -i %1 sh -c \"echo \'%2\' > %3\"").
            arg(getContainerName(container)).arg(file).arg(path);

    // qDebug().noquote() << "uploadTextFileToContainer\n" << script;

    SshConnection *client = connectToHost(sshParams(credentials));
    if (client->state() != SshConnection::State::Connected) {
        return fromSshConnectionErrorCode(client->errorState());
    }

    QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(script.toUtf8());

    if (!proc) {
        qCritical() << "Failed to create SshRemoteProcess, breaking.";
        return ErrorCode::SshRemoteProcessCreationError;
    }

    QEventLoop wait;
    int exitStatus = -1;

//    QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
//        qDebug() << "uploadTextFileToContainer started";
//    });

    QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
        //qDebug() << "Remote process exited with status" << status;
        exitStatus = status;
        wait.quit();
    });

    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, [proc](){
        qDebug().noquote() << proc->readAllStandardOutput();
    });

    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, [proc](){
        qDebug().noquote() << proc->readAllStandardError();
    });

    proc->start();

    if (exitStatus < 0) {
        wait.exec();
    }

    return fromSshProcessExitStatus(exitStatus);
}

QString ServerController::getTextFileFromContainer(DockerContainer container,
    const ServerCredentials &credentials, const QString &path, ErrorCode *errorCode)
{
    QString script = QString("sudo docker exec -i %1 sh -c \"cat \'%2\'\"").
            arg(getContainerName(container)).arg(path);

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

ErrorCode ServerController::signCert(DockerContainer container,
    const ServerCredentials &credentials, QString clientId)
{
    QString script_import = QString("sudo docker exec -i %1 bash -c \"cd /opt/amneziavpn_data && "
                             "easyrsa import-req /opt/amneziavpn_data/clients/%2.req %2\"")
            .arg(getContainerName(container)).arg(clientId);

    QString script_sign = QString("sudo docker exec -i %1 bash -c \"export EASYRSA_BATCH=1; cd /opt/amneziavpn_data && "
                                    "easyrsa sign-req client %2\"")
            .arg(getContainerName(container)).arg(clientId);

    QStringList script {script_import, script_sign};

    return runScript(container, sshParams(credentials), script.join("\n"));
}

ErrorCode ServerController::checkOpenVpnServer(DockerContainer container, const ServerCredentials &credentials)
{
    QString caCert = ServerController::getTextFileFromContainer(container,
        credentials, ServerController::caCertPath());
    QString taKey = ServerController::getTextFileFromContainer(container,
        credentials, ServerController::taKeyPath());

    if (!caCert.isEmpty() && !taKey.isEmpty()) {
        return ErrorCode::NoError;
    }
    else {
        return ErrorCode::ServerCheckFailed;
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
        return removeServer(credentials, Protocol::ShadowSocks);
    }
    else if (proto == Protocol::OpenVpn) {
        scriptFileName = ":/server_scripts/remove_container.sh";
        container = DockerContainer::OpenVpn;
    }
    else if (proto == Protocol::ShadowSocks) {
        scriptFileName = ":/server_scripts/remove_container.sh";
        container = DockerContainer::ShadowSocks;
    }
    else return ErrorCode::NotImplementedError;


    QString scriptData;

    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

    return runScript(container, sshParams(credentials), scriptData);
}

ErrorCode ServerController::setupServer(const ServerCredentials &credentials, Protocol proto)
{
    if (proto == Protocol::OpenVpn) {
        return ErrorCode::NoError;
        //return setupOpenVpnServer(credentials);
    }
    else if (proto == Protocol::ShadowSocks) {
        return setupShadowSocksServer(credentials);
    }
    else if (proto == Protocol::Any) {
        //return ErrorCode::NotImplementedError;

        // TODO: run concurently
        //setupOpenVpnServer(credentials);
        return setupShadowSocksServer(credentials);
    }

    return ErrorCode::NoError;
}

ErrorCode ServerController::setupOpenVpnServer(const ServerCredentials &credentials)
{
    QString scriptData;
    QString scriptFileName = ":/server_scripts/setup_openvpn_server.sh";
    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";

        if (data.contains("Automatically restart Docker daemon?")) {
            proc->write("yes\n");
        }
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(DockerContainer::OpenVpn, sshParams(credentials), scriptData, cbReadStdOut, cbReadStdErr);
    if (e) return e;
    QApplication::processEvents();

    if (stdOut.contains("port is already allocated")) return ErrorCode::ServerPortAlreadyAllocatedError;
    if (stdOut.contains("Error response from daemon")) return ErrorCode::ServerCheckFailed;

    return checkOpenVpnServer(DockerContainer::OpenVpn, credentials);
}

ErrorCode ServerController::setupShadowSocksServer(const ServerCredentials &credentials)
{
    // Setup openvpn part
    QString scriptData;
    QString scriptFileName = ":/server_scripts/setup_shadowsocks_server.sh";
    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";

        if (data.contains("Automatically restart Docker daemon?")) {
            proc->write("yes\n");
        }
    };
    auto cbReadStdErr = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
        stdOut += data + "\n";
    };

    ErrorCode e = runScript(DockerContainer::ShadowSocks, sshParams(credentials), scriptData, cbReadStdOut, cbReadStdErr);
    if (e) return e;

    // Create ss config
    QJsonObject ssConfig;
    ssConfig.insert("server", "0.0.0.0");
    ssConfig.insert("server_port", ssRemotePort());
    ssConfig.insert("local_port", ssContainerPort());
    ssConfig.insert("password", QString(QCryptographicHash::hash(credentials.password.toUtf8(), QCryptographicHash::Sha256).toHex()));
    //ssConfig.insert("password", credentials.password);
    ssConfig.insert("timeout", 60);
    ssConfig.insert("method", ssEncryption());
    QString configData = QJsonDocument(ssConfig).toJson();
    QString sSConfigPath = "/opt/amneziavpn_data/ssConfig.json";

    configData.replace("\"", "\\\"");
    //qDebug().noquote() << configData;

    uploadTextFileToContainer(DockerContainer::ShadowSocks, credentials, configData, sSConfigPath);

    // Start ss
    QString script = QString("sudo docker exec -d %1 sh -c \"ss-server -c %2\"").
            arg(getContainerName(DockerContainer::ShadowSocks)).arg(sSConfigPath);

    e = runScript(DockerContainer::ShadowSocks, sshParams(credentials), script);
    return e;
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
    QFile file(":/server_scripts/setup_firewall.sh");
    file.open(QIODevice::ReadOnly);

    QString script = file.readAll();
    return runScript(DockerContainer::OpenVpn, sshParams(credentials), script);
}
