#include "openvpnconfigurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>

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
//    easyRsaShPath.replace("C:\\", "/cygdrive/c/");
//    easyRsaShPath.replace("\\", "/");
    easyRsaShPath = "\"" + easyRsaShPath + "\"";

//    easyRsaShPath = "\"/cygdrive/c/Program Files (x86)/AmneziaVPN/easyrsa/easyrsa\"";

//    easyRsaShPath = "\"C:\\Program Files (x86)\\AmneziaVPN\\easyrsa\\easyrsa\"";
    qDebug().noquote() << "EasyRsa sh path" << easyRsaShPath;

    return easyRsaShPath;
//    return "\"/Program Files (x86)/AmneziaVPN/easyrsa/easyrsa\"";
#else
    return QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/easyrsa";
#endif
}

QProcessEnvironment OpenVpnConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");

#ifdef Q_OS_WIN
    pathEnvVar.clear();
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\cygwin;");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\openvpn\\i386;");
#else
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS");
#endif

    env.insert("PATH", pathEnvVar);
    //qDebug().noquote() << "ENV PATH" << pathEnvVar;
    return env;
}

ErrorCode OpenVpnConfigurator::initPKI(const QString &path)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
    p.setProcessEnvironment(prepareEnv());
    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"ash.exe %1\"").arg(getEasyRsaShPath() + " init-pki"));
    qDebug().noquote() << "EasyRsa tmp path" << path;
    qDebug().noquote() << "EasyRsa args" << p.nativeArguments();
#else
    p.setProgram(getEasyRsaShPath());
    p.setArguments(QStringList() << "init-pki");
#endif

    p.setWorkingDirectory(path);

    QObject::connect(&p, &QProcess::channelReadyRead, [&](){
        qDebug().noquote() <<  "Init PKI" << p.readAll();
    });

    p.start();
    p.waitForFinished();

    if (p.exitCode() == 0) return ErrorCode::NoError;
    else return ErrorCode::EasyRsaError;
}

ErrorCode OpenVpnConfigurator::genReq(const QString &path, const QString &clientId)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
    p.setProcessEnvironment(prepareEnv());
    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"ash.exe %1\"").arg(getEasyRsaShPath() + " gen-req " + clientId + " nopass"));
    qDebug().noquote() << "EasyRsa args" << p.nativeArguments();
#else
    p.setArguments(QStringList() << "gen-req" << clientId << "nopass");
    p.setProgram(getEasyRsaShPath());
#endif

    p.setWorkingDirectory(path);

    QObject::connect(&p, &QProcess::channelReadyRead, [&](){
        QString data = p.readAll();
        qDebug().noquote() << data;

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

    Q_UNUSED(errorCode)

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

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const ServerCredentials &credentials,
    Protocol proto, ErrorCode *errorCode)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = credentials.hostName;

    if (connData.privKey.isEmpty() || connData.request.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::EasyRsaExecutableMissing;
        return connData;
    }

    QString reqFileName = QString("/opt/amneziavpn_data/clients/%1.req").arg(connData.clientId);

    DockerContainer container;
    if (proto == Protocol::OpenVpn) container = DockerContainer::OpenVpn;
    else if (proto == Protocol::ShadowSocks) container = DockerContainer::ShadowSocks;
    else {
        if (errorCode) *errorCode = ErrorCode::InternalError;
        return connData;
    }

    ErrorCode e = ServerController::uploadTextFileToContainer(container, credentials, connData.request, reqFileName);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    e = ServerController::signCert(container, credentials, connData.clientId);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.caCert = ServerController::getTextFileFromContainer(container, credentials, ServerController::caCertPath(), &e);
    connData.clientCert = ServerController::getTextFileFromContainer(container, credentials, ServerController::clientCertPath() + QString("%1.crt").arg(connData.clientId), &e);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.taKey = ServerController::getTextFileFromContainer(container, credentials, ServerController::taKeyPath(), &e);

    if (connData.caCert.isEmpty() || connData.clientCert.isEmpty() || connData.taKey.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::RemoteProcessCrashError;
    }

    ServerController::setupServerFirewall(credentials);

    return connData;
}

Settings &OpenVpnConfigurator::m_settings()
{
    static Settings s;
    return s;
}

QString OpenVpnConfigurator::genOpenVpnConfig(const ServerCredentials &credentials,
    Protocol proto, ErrorCode *errorCode)
{
    QFile configTemplFile;
    if (proto == Protocol::OpenVpn)
        configTemplFile.setFileName(":/server_scripts/template_openvpn.ovpn");
    else if (proto == Protocol::ShadowSocks) {
        configTemplFile.setFileName(":/server_scripts/template_shadowsocks.ovpn");
    }

    configTemplFile.open(QIODevice::ReadOnly);
    QString config = configTemplFile.readAll();

    ConnectionData connData = prepareOpenVpnConfig(credentials, proto, errorCode);

    if (proto == Protocol::OpenVpn)
        config.replace("$PROTO", "udp");
    else if (proto == Protocol::ShadowSocks) {
        config.replace("$PROTO", "tcp");
        config.replace("$LOCAL_PROXY_PORT", QString::number(ServerController::ssContainerPort()));
    }

    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    if (m_settings().customRouting()) {
        config.replace("redirect-gateway def1 bypass-dhcp", "");
    }

    if (proto == Protocol::ShadowSocks) {
        config.replace("$REMOTE_HOST", "10.8.0.1");
    }
    else {
        config.replace("$REMOTE_HOST", connData.host);
    }
    config.replace("$REMOTE_PORT", "1194");
    config.replace("$CA_CERT", connData.caCert);
    config.replace("$CLIENT_CERT", connData.clientCert);
    config.replace("$PRIV_KEY", connData.privKey);
    config.replace("$TA_KEY", connData.taKey);

#ifdef Q_OS_MAC
    config.replace("block-outside-dns", "");
#endif
    //qDebug().noquote() << config;
    return config;
}

QString OpenVpnConfigurator::convertOpenSShKey(const QString &key)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    QTemporaryFile tmp;
    tmp.setAutoRemove(false);
    tmp.open();
    tmp.write(key.toUtf8());
    tmp.close();

    // ssh-keygen -p -P "" -N "" -m pem -f id_ssh

#ifdef Q_OS_WIN
    p.setProcessEnvironment(prepareEnv());
    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"ssh-keygen.exe -p -P \"\" -N \"\" -m pem -f \"%1\"\"").arg(tmp.fileName()));
#else
    p.setProgram("ssh-keygen");
    p.setArguments(QStringList() << "-p" << "-P" << "" << "-N" << "" << "-m" << "pem" << "-f" << tmp.fileName());
#endif

    p.start();
    p.waitForFinished();

    qDebug().noquote() << "OpenVpnConfigurator::convertOpenSShKey" << p.exitCode() << p.exitStatus() << p.readAll();

    tmp.open();

    return tmp.readAll();
}
