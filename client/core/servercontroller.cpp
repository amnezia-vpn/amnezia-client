#include "servercontroller.h"

#include <QFile>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>

//#include "sshclient.h"
//#include "sshprocess.h"

#include "sshconnectionmanager.h"
#include "sshremoteprocess.h"

using namespace QSsh;

bool ServerController::runScript(const SshConnectionParameters &sshParams, QString script)
{
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    SshConnection *client = connectToHost(sshParams);

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
            return false;
        }

        QEventLoop wait;

        QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
            qDebug() << "Command started";
        });

        QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
            qDebug() << "Remote process exited with status" << status;
            wait.quit();
        });

        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, [proc](){
            qDebug().noquote() << proc->readAllStandardOutput();
        });

        QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, [proc](){
            qDebug().noquote() << proc->readAllStandardError();
        });

        proc->start();
        if (i < lines.count() - 1) {
            wait.exec();
        }
    }

    qDebug() << "ServerController::runScript finished";

//    client->disconnectFromHost();

//    client->deleteLater();
    return true;
}

void ServerController::uploadTextFileToContainer(const SshConnectionParameters &sshParams,
                                     QString &file, const QString &path)
{
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    QString script = QString("docker exec -i amneziavpn sh -c \"echo \'%1\' > %2\"").
            arg(file).arg(path);

    qDebug().noquote() << script;

    SshConnection *client = connectToHost(sshParams);
    QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(script.toUtf8());

    if (!proc) {
        qCritical() << "Failed to create SshRemoteProcess, breaking.";
        return;
    }

    QEventLoop wait;

    QObject::connect(proc.data(), &SshRemoteProcess::started, &wait, [](){
        qDebug() << "Command started";
    });

    QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int status){
        qDebug() << "Remote process exited with status" << status;
        wait.quit();
    });

    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardOutput, [proc](){
        qDebug().noquote() << proc->readAllStandardOutput();
    });

    QObject::connect(proc.data(), &SshRemoteProcess::readyReadStandardError, [proc](){
        qDebug().noquote() << proc->readAllStandardError();
    });

    proc->start();
    wait.exec();
}

QString ServerController::getTextFileFromContainer(const SshConnectionParameters &sshParams, const QString &path)
{
    QString script = QString("docker exec -i amneziavpn sh -c \"cat \'%1\'\"").
            arg(path);

    SshConnection *client = connectToHost(sshParams);
    QSharedPointer<SshRemoteProcess> proc = client->createRemoteProcess(script.toUtf8());

    QEventLoop wait;

    QObject::connect(proc.data(), &SshRemoteProcess::closed, &wait, [&](int ){
        wait.quit();
    });

    proc->start();
    wait.exec();

    return proc->readAllStandardOutput();
}

bool ServerController::signCert(const SshConnectionParameters &sshParams, QString clientId)
{
    QString script_import = QString("docker exec -i amneziavpn bash -c \"cd /opt/amneziavpn_data && "
                             "easyrsa import-req /opt/amneziavpn_data/clients/%1.req %1 &>/dev/null\"")
            .arg(clientId);

    QString script_sign = QString("docker exec -i amneziavpn bash -c \"export EASYRSA_BATCH=1; cd /opt/amneziavpn_data && "
                                    "easyrsa sign-req client %1 &>/dev/null\"")
            .arg(clientId);

    QStringList script {script_import, script_sign};

    return runScript(sshParams, script.join("\n"));
}

bool ServerController::removeServer(const SshConnectionParameters &sshParams, ServerController::ServerType sType)
{
    QString scriptFileName;

    if (sType == OpenVPN) {
        scriptFileName = ":/server_scripts/remove_openvpn_server.sh";
    }

    QString scriptData;

    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) {
        return false;
    }

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return false;

    return runScript(sshParams, scriptData);
}

bool ServerController::setupServer(const SshConnectionParameters &sshParams, ServerController::ServerType sType)
{
    QString scriptFileName;

    if (sType == OpenVPN) {
        scriptFileName = ":/server_scripts/setup_openvpn_server.sh";
    }

    QString scriptData;

    QFile file(scriptFileName);
    if (! file.open(QIODevice::ReadOnly)) {
        return false;
    }

    scriptData = file.readAll();
    if (scriptData.isEmpty()) return false;

    return runScript(sshParams, scriptData);
}

SshConnection *ServerController::connectToHost(const SshConnectionParameters &sshParams)
{
    SshConnection *client = acquireConnection(sshParams);
    //QPointer<SshConnection> client = new SshConnection(serverInfo);

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
