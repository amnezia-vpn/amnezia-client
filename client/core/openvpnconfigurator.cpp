#include "openvpnconfigurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QDebug>

QString OpenVpnConfigurator::getRandomString(int len)
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;
    for(int i=0; i<len; ++i) {
        quint32 index = QRandomGenerator::global()->generate() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}

QString OpenVpnConfigurator::getEasyRsaShPath()
{
    QString easyRsaShPath = QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\easyrsa\\easyrsa";
    easyRsaShPath.replace(":", "");
    easyRsaShPath.replace("\\", "/");
    easyRsaShPath.prepend("/");

    return easyRsaShPath;
}

QProcessEnvironment OpenVpnConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\easyrsa\\bin;");

    env.insert("PATH", pathEnvVar);
    return env;
}

void OpenVpnConfigurator::initPKI(const QString &path)
{
#ifdef Q_OS_WIN
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProcessEnvironment(prepareEnv());

    QString command = QString("sh.exe");

    p.setNativeArguments(getEasyRsaShPath() + " init-pki");

    p.setWorkingDirectory(path);

    p.start(command);
    p.waitForFinished();
    qDebug().noquote() << p.readAll();

#endif
}

QString OpenVpnConfigurator::genReq(const QString &path, const QString &clientId)
{
#ifdef Q_OS_WIN
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProcessEnvironment(prepareEnv());

    QString command = QString("sh.exe");

    p.setNativeArguments(getEasyRsaShPath() + " gen-req " + clientId + " nopass");

    p.setWorkingDirectory(path);

    QObject::connect(&p, &QProcess::channelReadyRead, [&](){
        QString data = p.readAll();
        qDebug().noquote() << data;

        if (data.contains("Common Name (eg: your user, host, or server name)")) {
            p.write("\n");
        }
    });

    p.start(command);
    p.waitForFinished();
//    qDebug().noquote() << p.readAll();

    return "";
#endif
}


OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::createCertRequest()
{
    OpenVpnConfigurator::ConnectionData connData;
    connData.clientId = getRandomString(32);

    QTemporaryDir dir;
//    if (dir.isValid()) {
//            // dir.path() returns the unique directory path
//    }

    QString path = dir.path();

    initPKI(path);
    genReq(path, connData.clientId);


    QFile req(path + "/pki/reqs/" + connData.clientId + ".req");
    req.open(QIODevice::ReadOnly);
    connData.request = req.readAll();

    QFile key(path + "/pki/private/" + connData.clientId + ".key");
    key.open(QIODevice::ReadOnly);
    connData.privKey = key.readAll();

    qDebug().noquote() << connData.request;
    qDebug().noquote() << connData.privKey;


    return connData;
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const QSsh::SshConnectionParameters &sshParams)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = sshParams.host;

    QString reqFileName = QString("/opt/amneziavpn_data/clients/%1.req").arg(connData.clientId);
    ServerController::uploadTextFileToContainer(sshParams, connData.request, reqFileName);

    ServerController::signCert(sshParams, connData.clientId);

    connData.caCert = ServerController::getTextFileFromContainer(sshParams, QString("/opt/amneziavpn_data/pki/ca.crt"));
    connData.clientCert = ServerController::getTextFileFromContainer(sshParams, QString("/opt/amneziavpn_data/pki/issued/%1.crt").arg(connData.clientId));
    connData.taKey = ServerController::getTextFileFromContainer(sshParams, QString("/opt/amneziavpn_data/ta.key"));


    return connData;
}

QString OpenVpnConfigurator::genOpenVpnConfig(const QSsh::SshConnectionParameters &sshParams)
{
    QFile configTemplFile(":/server_scripts/template.ovpn");
    configTemplFile.open(QIODevice::ReadOnly);
    QString config = configTemplFile.readAll();

    ConnectionData connData = prepareOpenVpnConfig(sshParams);

    config.replace("$PROTO", "udp");
    config.replace("$REMOTE_HOST", connData.host);
    config.replace("$REMOTE_PORT", "1194");
    config.replace("$CA_CERT", connData.caCert);
    config.replace("$CLIENT_CERT", connData.clientCert);
    config.replace("$PRIV_KEY", connData.privKey);
    config.replace("$TA_KEY", connData.taKey);

    return config;
}
