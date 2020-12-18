#include "Ssh.hpp"
#include <QDir>
#include <QDebug>
#include <QLoggingCategory>

Ssh::Ssh(QObject *parent) : QObject(parent) {
    //关掉qtc.ssh中的各种打印信息
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    mParams.host="ftb.autoio.org";
    mParams.userName = "ftb";
    mParams.port = 11122;

    mParams.privateKeyFile = QDir::homePath() +  QStringLiteral("/.ssh/id_rsa");
    mParams.timeout = 5;
    mParams.authenticationType = SshConnectionParameters::AuthenticationTypePublicKey;
    mParams.options = SshIgnoreDefaultProxy;
    mParams.hostKeyCheckingMode = SshHostKeyCheckingNone;

    mConnections = std::make_shared<SshConnection>(mParams);
    connect(mConnections.get(), &SshConnection::error, [&](QSsh::SshError){
        qWarning() << "Error: " << mConnections->errorString();
    });
    connect(mConnections.get(), &SshConnection::connected, [&](){
        qWarning() << "Connected";
        create();
    });
    connect(mConnections.get(), &SshConnection::disconnected, [](){
        qWarning() << "Disconnected";
    });
    connect(mConnections.get(), &SshConnection::dataAvailable, [](const QString &message){
        qWarning() << "Message: " << message;
    });
}

void Ssh::connectToHost() {
    mConnections->connectToHost();
}

void Ssh::create() {
    mRemoteProcess = mConnections->createRemoteProcess(QString::fromLatin1("/bin/ls -a").toUtf8());
    if (!mRemoteProcess) {
        qWarning() << QLatin1String("Error: UnmRemoteProcess SSH connection creates remote process.");
        return;
    }
    connect(mRemoteProcess.data(), &SshRemoteProcess::started, [&](){
        qWarning() << "started";
    });
    connect(mRemoteProcess.data(), &SshRemoteProcess::readyReadStandardOutput, [&](){
        qWarning() << "StandardOutput";
        qWarning() << QString::fromLatin1(mRemoteProcess->readAllStandardOutput()).split('\n');
    });
    connect(mRemoteProcess.data(), &SshRemoteProcess::readyReadStandardError, [&](){
        qWarning() << "StandardError" << mRemoteProcess->readAllStandardError();
    });
    connect(mRemoteProcess.data(), &SshRemoteProcess::closed, [&](int exitStatus){
        qWarning() << "Exit" << exitStatus;
    });
    mRemoteProcess->start();
}

