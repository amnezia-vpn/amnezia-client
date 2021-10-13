#include "openvpn_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QJsonObject>

#include "core/server_defs.h"
#include "containers/containers_defs.h"
#include "core/scripts_registry.h"
#include "utils.h"

QString OpenVpnConfigurator::getEasyRsaShPath()
{
#ifdef Q_OS_WIN
    // easyrsa sh path should looks like
    QString easyRsaShPath = QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\easyrsa\\easyrsa";
    qDebug().noquote() << "EasyRsa sh path" << easyRsaShPath;

    return easyRsaShPath;

#else
    return QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/easyrsa";
#endif
}

QProcessEnvironment OpenVpnConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");

#if defined Q_OS_WIN
    pathEnvVar.clear();
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\cygwin;");
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\openvpn;");
#elif defined Q_OS_MAC
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS");
#elif defined Q_OS_LINUX
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/openvpn");
#endif

    env.insert("PATH", pathEnvVar);
    //qDebug().noquote() << "ENV PATH" << pathEnvVar;
    return env;
}

ErrorCode OpenVpnConfigurator::initPKI(const QString &path)
{
#ifndef Q_OS_IOS
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
    p.setProcessEnvironment(prepareEnv());
    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"ash.exe \"%1\" %2\"").arg(getEasyRsaShPath()).arg("init-pki"));
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
#else
    return ErrorCode::NotImplementedError;
#endif
}

ErrorCode OpenVpnConfigurator::genReq(const QString &path, const QString &clientId)
{
#ifndef Q_OS_IOS
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
    p.setProcessEnvironment(prepareEnv());
    p.setProgram("cmd.exe");
    p.setNativeArguments(QString("/C \"ash.exe \"%1\" %2 %3 %4\"")
                         .arg(getEasyRsaShPath())
                         .arg("gen-req").arg(clientId).arg("nopass"));

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
#else
    return ErrorCode::NotImplementedError;
#endif
}


OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::createCertRequest()
{
    OpenVpnConfigurator::ConnectionData connData;
    connData.clientId = Utils::getRandomString(32);

    QTemporaryDir dir;
    //    if (dir.isValid()) {
    //            // dir.path() returns the unique directory path
    //    }

    QString path = dir.path();

    initPKI(path);
    ErrorCode errorCode = genReq(path, connData.clientId);

    Q_UNUSED(errorCode)

#if defined Q_OS_LINUX
    if (!QDir(path).exists())
    {
        QDir().mkdir(path);
    }

    if (!QDir(path + "/pki/").exists())
    {
        QDir().mkdir(path + "/pki/");
        QDir().mkdir(path + "/pki/reqs/");
        QDir().mkdir(path + "/pki/private/");
    }
#endif

    QFile req(path + "/pki/reqs/" + connData.clientId + ".req");
    req.open(QIODevice::ReadWrite);
    connData.request = req.readAll();

    QFile key(path + "/pki/private/" + connData.clientId + ".key");
    key.open(QIODevice::ReadWrite);
    connData.privKey = key.readAll();

    //    qDebug().noquote() << connData.request;
    //    qDebug().noquote() << connData.privKey;

    return connData;
}

OpenVpnConfigurator::ConnectionData OpenVpnConfigurator::prepareOpenVpnConfig(const ServerCredentials &credentials,
    DockerContainer container, ErrorCode *errorCode)
{
    OpenVpnConfigurator::ConnectionData connData = OpenVpnConfigurator::createCertRequest();
    connData.host = credentials.hostName;

    if (connData.privKey.isEmpty() || connData.request.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::EasyRsaExecutableMissing;
        return connData;
    }

    QString reqFileName = QString("%1/%2.req").
            arg(amnezia::protocols::openvpn::clientsDirPath).
            arg(connData.clientId);

    ErrorCode e = ServerController::uploadTextFileToContainer(container, credentials, connData.request, reqFileName);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    e = signCert(container, credentials, connData.clientId);
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.caCert = ServerController::getTextFileFromContainer(container, credentials, amnezia::protocols::openvpn::caCertPath, &e);
    connData.clientCert = ServerController::getTextFileFromContainer(container, credentials,
        QString("%1/%2.crt").arg(amnezia::protocols::openvpn::clientCertPath).arg(connData.clientId), &e);

    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.taKey = ServerController::getTextFileFromContainer(container, credentials, amnezia::protocols::openvpn::taKeyPath, &e);

    if (connData.caCert.isEmpty() || connData.clientCert.isEmpty() || connData.taKey.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::RemoteProcessCrashError;
    }

    return connData;
}

Settings &OpenVpnConfigurator::m_settings()
{
    static Settings s;
    return s;
}

QString OpenVpnConfigurator::genOpenVpnConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QString config = ServerController::replaceVars(amnezia::scriptData(ProtocolScriptType::openvpn_template, container),
            ServerController::genVarsForScript(credentials, container, containerConfig));

    ConnectionData connData = prepareOpenVpnConfig(credentials, container, errorCode);
    if (errorCode && *errorCode) {
        return "";
    }

    config.replace("$OPENVPN_CA_CERT", connData.caCert);
    config.replace("$OPENVPN_CLIENT_CERT", connData.clientCert);
    config.replace("$OPENVPN_PRIV_KEY", connData.privKey);

    if (config.contains("$OPENVPN_TA_KEY")) {
        config.replace("$OPENVPN_TA_KEY", connData.taKey);
    }
    else {
        config.replace("<tls-auth>", "");
        config.replace("</tls-auth>", "");
    }

#if defined Q_OS_MAC || defined(Q_OS_LINUX)
    config.replace("block-outside-dns", "");
#endif

    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    return QJsonDocument(jConfig).toJson();
}

QString OpenVpnConfigurator::processConfigWithLocalSettings(QString config)
{
    if (m_settings().routeMode() != Settings::VpnAllSites) {
        config.replace("redirect-gateway def1 bypass-dhcp", "");
    }
    else {
        if(!config.contains("redirect-gateway def1 bypass-dhcp")) {
            config.append("redirect-gateway def1 bypass-dhcp\n");
        }
    }

#if defined Q_OS_MAC || defined(Q_OS_LINUX)
    config.replace("block-outside-dns", "");
    QString dnsConf = QString(
                "\nscript-security 2\n"
                "up %1/update-resolv-conf.sh\n"
                "down %1/update-resolv-conf.sh\n").
            arg(qApp->applicationDirPath());

    config.append(dnsConf);
#endif

    return config;
}

QString OpenVpnConfigurator::processConfigWithExportSettings(QString config)
{
    if(!config.contains("redirect-gateway def1 bypass-dhcp")) {
        config.append("redirect-gateway def1 bypass-dhcp\n");
    }

#if defined Q_OS_MAC || defined(Q_OS_LINUX)
    config.replace("block-outside-dns", "");
#endif

    return config;
}

ErrorCode OpenVpnConfigurator::signCert(DockerContainer container,
    const ServerCredentials &credentials, QString clientId)
{
    QString script_import = QString("sudo docker exec -i %1 bash -c \"cd /opt/amnezia/openvpn && "
                             "easyrsa import-req %2/%3.req %3\"")
            .arg(ContainerProps::containerToString(container))
            .arg(amnezia::protocols::openvpn::clientsDirPath)
            .arg(clientId);

    QString script_sign = QString("sudo docker exec -i %1 bash -c \"export EASYRSA_BATCH=1; cd /opt/amnezia/openvpn && "
                                    "easyrsa sign-req client %2\"")
            .arg(ContainerProps::containerToString(container))
            .arg(clientId);

    QStringList scriptList {script_import, script_sign};
    QString script = ServerController::replaceVars(scriptList.join("\n"), ServerController::genVarsForScript(credentials, container));

    return ServerController::runScript(credentials, script);
}
