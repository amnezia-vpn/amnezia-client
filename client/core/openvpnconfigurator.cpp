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
#ifdef Q_OS_WIN
    // easyrsa sh path should looks like
    // "/Program Files (x86)/AmneziaVPN/easyrsa/easyrsa"
    QString easyRsaShPath = QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\easyrsa\\easyrsa";
    easyRsaShPath.replace("C:\\", "");
    easyRsaShPath.replace("\\", "/");
    easyRsaShPath.prepend("/");

    //return "\"" + easyRsaShPath + "\"";
    return "\"/Program Files (x86)/AmneziaVPN/easyrsa/easyrsa\"";
#else
    return QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/easyrsa";
#endif
}

QProcessEnvironment OpenVpnConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");

#ifdef Q_OS_WIN
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\easyrsa\\bin;");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\openvpn\\i386;");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\openvpn\\x64;");
#else
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS");
#endif

    env.insert("PATH", pathEnvVar);
    return env;
}

ErrorCode OpenVpnConfigurator::initPKI(const QString &path)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProcessEnvironment(prepareEnv());

#ifdef Q_OS_WIN
    //p.setProgram("sh.exe");
    //p.setNativeArguments(getEasyRsaShPath() + " init-pki");

    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"sh.exe %1\"").arg(getEasyRsaShPath() + " init-pki"));
#else
    p.setProgram(getEasyRsaShPath());
    p.setArguments(QStringList() << "init-pki");
#endif

    p.setWorkingDirectory(path);

//    QObject::connect(&p, &QProcess::channelReadyRead, [&](){
//        qDebug().noquote() << p.readAll();
//    });

    p.start();
    p.waitForFinished();

    if (p.exitCode() == 0) return ErrorCode::NoError;
    else return ErrorCode::EasyRsaError;
}

ErrorCode OpenVpnConfigurator::genReq(const QString &path, const QString &clientId)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.setProcessEnvironment(prepareEnv());

#ifdef Q_OS_WIN
    //p.setProgram("sh.exe");
    //p.setNativeArguments(getEasyRsaShPath() + " gen-req " + clientId + " nopass");

    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"sh.exe %1\"").arg(getEasyRsaShPath() + " gen-req " + clientId + " nopass"));
#else
    p.setArguments(QStringList() << "gen-req" << clientId << "nopass");
    p.setProgram(getEasyRsaShPath());
#endif

    p.setWorkingDirectory(path);

    QObject::connect(&p, &QProcess::channelReadyRead, [&](){
        QString data = p.readAll();
        //qDebug().noquote() << data;

        if (data.contains("Common Name (eg: your user, host, or server name)")) {
            p.write("\n");
        }
    });

    p.start();
    p.waitForFinished();

    if (p.exitCode() == 0) return ErrorCode::NoError;
    else return ErrorCode::EasyRsaError;
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
    ErrorCode errorCode = genReq(path, connData.clientId);


    QFile req(path + "/pki/reqs/" + connData.clientId + ".req");
    req.open(QIODevice::ReadOnly);
    connData.request = req.readAll();

    QFile key(path + "/pki/private/" + connData.clientId + ".key");
    key.open(QIODevice::ReadOnly);
    connData.privKey = key.readAll();

//    qDebug().noquote() << connData.request;
//    qDebug().noquote() << connData.privKey;

    return connData;
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const ServerCredentials &credentials, ErrorCode *errorCode)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = credentials.hostName;

    if (connData.privKey.isEmpty() || connData.request.isEmpty()) {
       *errorCode = ErrorCode::EasyRsaExecutableMissing;
        return connData;
    }

    QString reqFileName = QString("/opt/amneziavpn_data/clients/%1.req").arg(connData.clientId);
    ErrorCode e = ServerController::uploadTextFileToContainer(credentials, connData.request, reqFileName);
    if (e) {
        *errorCode = e;
        return connData;
    }

    ServerController::signCert(credentials, connData.clientId);

    connData.caCert = ServerController::getTextFileFromContainer(credentials, ServerController::caCertPath(), &e);
    connData.clientCert = ServerController::getTextFileFromContainer(credentials, ServerController::clientCertPath() + QString("%1.crt").arg(connData.clientId), &e);
    if (e) {
        *errorCode = e;
        return connData;
    }

    connData.taKey = ServerController::getTextFileFromContainer(credentials, ServerController::taKeyPath(), &e);

    return connData;
}

QString OpenVpnConfigurator::genOpenVpnConfig(const ServerCredentials &credentials, ErrorCode *errorCode)
{
    QFile configTemplFile(":/server_scripts/template.ovpn");
    configTemplFile.open(QIODevice::ReadOnly);
    QString config = configTemplFile.readAll();

    ConnectionData connData = prepareOpenVpnConfig(credentials, errorCode);

    config.replace("$PROTO", "udp");
    config.replace("$REMOTE_HOST", connData.host);
    config.replace("$REMOTE_PORT", "1194");
    config.replace("$CA_CERT", connData.caCert);
    config.replace("$CLIENT_CERT", connData.clientCert);
    config.replace("$PRIV_KEY", connData.privKey);
    config.replace("$TA_KEY", connData.taKey);

    return config;
}
