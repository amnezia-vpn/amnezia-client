#include "servercontroller.h"

#include <QFile>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>

#include "sshconnectionmanager.h"


using namespace QSsh;

ErrorCode ServerController::runScript(const SshConnectionParameters &sshParams, QString script)
{
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    SshConnection *client = connectToHost(sshParams);
    if (client->state() != SshConnection::State::Connected) {
        return fromSshConnectionErrorCode(client->errorState());
    }

    script.replace("\r", "");

    qDebug() << "Run script";

    const QStringList &lines = script.split("\n", QString::SkipEmptyParts);
    for (int i = 0; i < lines.count(); i++) {
        const QString &line = lines.at(i);
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
        int exitStatus;

        //        QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
        //            qDebug() << "Command started";
        //        });

        QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
            exitStatus = status;
            //qDebug() << "Remote process exited with status" << status;
            wait.quit();
        });

        //        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, [proc](){
        //            QString s = proc->readAllStandardOutput();
        //            if (s != "." && !s.isEmpty()) {
        //                qDebug().noquote() << s;
        //            }
        //        });

        //        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, [proc](){
        //            QString s = proc->readAllStandardError();
        //            if (s != "." && !s.isEmpty()) {
        //                qDebug().noquote() << s;
        //            }
        //        });

        proc->start();

        if (i < lines.count() - 1) {
            wait.exec();
        }

        if (SshRemoteProcess::ExitStatus(exitStatus) != QSsh::SshRemoteProcess::ExitStatus::NormalExit) {
            return fromSshProcessExitStatus(exitStatus);
        }
    }

    qDebug() << "ServerController::runScript finished\n";
    return ErrorCode::NoError;
}

ErrorCode ServerController::uploadTextFileToContainer(const ServerCredentials &credentials,
                                                      QString &file, const QString &path)
{
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    QString script = QString("docker exec -i amneziavpn sh -c \"echo \'%1\' > %2\"").
            arg(file).arg(path);

    qDebug().noquote() << script;

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
    int exitStatus = 0;

    //    QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
    //        qDebug() << "Command started";
    //    });

    QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
        //qDebug() << "Remote process exited with status" << status;
        exitStatus = status;
        wait.quit();
    });

    //    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, [proc](){
    //        qDebug().noquote() << proc->readAllStandardOutput();
    //    });

    //    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, [proc](){
    //        qDebug().noquote() << proc->readAllStandardError();
    //    });

    proc->start();
    wait.exec();

    return fromSshProcessExitStatus(exitStatus);
}

QString ServerController::getTextFileFromContainer(const ServerCredentials &credentials, const QString &path,
                                                   ErrorCode *errorCode)
{
    QString script = QString("docker exec -i amneziavpn sh -c \"cat \'%1\'\"").
            arg(path);

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

    proc->start();
    wait.exec();

    if (SshRemoteProcess::ExitStatus(exitStatus) != QSsh::SshRemoteProcess::ExitStatus::NormalExit) {
        if (errorCode) *errorCode = fromSshProcessExitStatus(exitStatus);
    }

    return proc->readAllStandardOutput();
}

ErrorCode ServerController::signCert(const ServerCredentials &credentials, QString clientId)
{
    QString script_import = QString("docker exec -i amneziavpn bash -c \"cd /opt/amneziavpn_data && "
                             "easyrsa import-req /opt/amneziavpn_data/clients/%1.req %1 &>/dev/null\"")
            .arg(clientId);

    QString script_sign = QString("docker exec -i amneziavpn bash -c \"export EASYRSA_BATCH=1; cd /opt/amneziavpn_data && "
                                    "easyrsa sign-req client %1 &>/dev/null\"")
            .arg(clientId);

    QStringList script {script_import, script_sign};

    return runScript(sshParams(credentials), script.join("\n"));
}

ErrorCode ServerController::checkOpenVpnServer(const ServerCredentials &credentials)
{
    QString caCert = ServerController::getTextFileFromContainer(credentials, ServerController::caCertPath());
    QString taKey = ServerController::getTextFileFromContainer(credentials, ServerController::taKeyPath());

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
    }
}

ErrorCode ServerController::fromSshProcessExitStatus(int exitStatus)
{
    switch (SshRemoteProcess::ExitStatus(exitStatus)) {
    case(SshRemoteProcess::ExitStatus::NormalExit): return ErrorCode::NoError;
    case(SshRemoteProcess::ExitStatus::FailedToStart): return ErrorCode::FailedToStartRemoteProcessError;
    case(SshRemoteProcess::ExitStatus::CrashExit): return ErrorCode::RemoteProcessCrashError;
    }
}

SshConnectionParameters ServerController::sshParams(const ServerCredentials &credentials)
{
    QSsh::SshConnectionParameters sshParams;
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;
    sshParams.host = credentials.hostName;
    sshParams.userName = credentials.userName;
    sshParams.password = credentials.password;
    sshParams.timeout = 10;
    sshParams.port = credentials.port;
    sshParams.hostKeyCheckingMode = QSsh::SshHostKeyCheckingMode::SshHostKeyCheckingNone;

    return sshParams;
}

ErrorCode ServerController::removeServer(const ServerCredentials &credentials, Protocol proto)
{
    QString scriptFileName;

    if (proto == Protocol::OpenVpn || proto == Protocol::Any) {
        scriptFileName = ":/server_scripts/remove_openvpn_server.sh";
    }

    QString scriptData;

    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

    return runScript(sshParams(credentials), scriptData);
}

ErrorCode ServerController::setupServer(const ServerCredentials &credentials, Protocol proto)
{
    if (proto == Protocol::OpenVpn) {
        return setupOpenVpnServer(credentials);
    }
    else if (proto == Protocol::ShadowSocks) {
        return setupShadowSocksServer(credentials);
    }
    else if (proto == Protocol::Any) {
        // TODO: run concurently
        return setupOpenVpnServer(credentials);
        //setupShadowSocksServer(credentials);
    }

    return ErrorCode::NotImplementedError;
}

ErrorCode ServerController::setupOpenVpnServer(const ServerCredentials &credentials)
{
    QString scriptData;
    QString scriptFileName = ":/server_scripts/setup_openvpn_server.sh";
    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) return ErrorCode::InternalError;

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return ErrorCode::InternalError;

    ErrorCode e = runScript(sshParams(credentials), scriptData);
    if (e) return e;

    //return ok;
    return checkOpenVpnServer(credentials);
}

ErrorCode ServerController::setupShadowSocksServer(const ServerCredentials &credentials)
{
    Q_UNUSED(credentials)

    return ErrorCode::NotImplementedError;
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
