#include "shadowsocksvpnprotocol.h"

#include "logger.h"
#include "utilities.h"
#include "containers/containers_defs.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

ShadowSocksVpnProtocol::ShadowSocksVpnProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    readShadowSocksConfiguration(configuration);
    m_routeGateway = "10.33.0.1";
    m_vpnGateway = "10.33.0.1";
}

ShadowSocksVpnProtocol::~ShadowSocksVpnProtocol()
{
    qDebug() << "ShadowSocksVpnProtocol::~ShadowSocksVpnProtocol";
    ShadowSocksVpnProtocol::stop();
    QThread::msleep(200);
#ifndef Q_OS_IOS
    m_ssProcess.close();
#endif
}

ErrorCode ShadowSocksVpnProtocol::start()
{

     qDebug().noquote() << "ShadowSocksVpnProtocol shadowSocksExecPath():" <<shadowSocksExecPath();

    if (!QFileInfo::exists(shadowSocksExecPath())) {
        setLastError(ErrorCode::ShadowSocksExecutableMissing);
        return lastError();
    }


#ifndef Q_OS_IOS
    if (Utils::processIsRunning(Utils::executable("ss-local", false))) {
        Utils::killProcessByName(Utils::executable("ss-local", false));
    }

#ifdef QT_DEBUG
    m_shadowSocksCfgFile.setAutoRemove(false);
#endif
    m_shadowSocksCfgFile.open();
    m_shadowSocksCfgFile.write(QJsonDocument(m_shadowSocksConfig).toJson());
    m_shadowSocksCfgFile.close();

#ifdef Q_OS_LINUX
    QStringList args = QStringList() << "-c" << m_shadowSocksCfgFile.fileName();
#else
    QStringList args = QStringList() << "-c" << m_shadowSocksCfgFile.fileName()
                                     << "--no-delay";
#endif

    qDebug().noquote() << "ShadowSocksVpnProtocol::start()"
                       << shadowSocksExecPath() << args.join(" ");

    m_ssProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_ssProcess.setProgram(shadowSocksExecPath());
    m_ssProcess.setArguments(args);

    connect(&m_ssProcess, &QProcess::readyReadStandardOutput, this, [this](){
        qDebug().noquote() << "ss-local:" << m_ssProcess.readAllStandardOutput();
    });

    connect(&m_ssProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus){
        setConnectionState(VpnProtocol::Disconnected);
        if (exitStatus != QProcess::NormalExit){
            emit protocolError(amnezia::ErrorCode::ShadowSocksExecutableCrashed);
            stop();
        }
        if (exitCode !=0 ){
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_ssProcess.start();
    m_ssProcess.waitForStarted();

    if (m_ssProcess.state() == QProcess::ProcessState::Running) {
        setConnectionState(VpnConnectionState::Connecting);
        QThread::msleep(1000);
        return startTun2Sock();
    }
    else return ErrorCode::ShadowSocksExecutableMissing;
#else
    return ErrorCode::NotImplementedError;
#endif
}

ErrorCode ShadowSocksVpnProtocol::startTun2Sock()
{
    if (!QFileInfo::exists(tun2SocksExecPath())) {
        setLastError(ErrorCode::ShadowSocksExecutableMissing);
        return lastError();
    }


#ifndef Q_OS_IOS
    if (Utils::processIsRunning(Utils::executable("tun2socks", false))) {
        Utils::killProcessByName(Utils::executable("tun2socks", false));
    }

    QString SSConStr = "socks5://127.0.0.1:" + QString::number(m_localPort);

#ifdef Q_OS_WIN
    QStringList args = QStringList() << "-device" << "tun://tun2" <<
                       "-proxy" << SSConStr <<
                       "-tun-post-up" <<
                       "netsh interface ip set address name=\"tun2\" static 10.33.0.1 255.255.255.0 10.33.0.1";
#else
    QStringList args = QStringList() << "-device" << "tun://tun2" <<
                       "-proxy" << SSConStr;
#endif

    qDebug().noquote() << "ShadowSocksVpnProtocol::startTun2Sock()"
                       << tun2SocksExecPath() << args.join(" ");

    m_t2sProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_t2sProcess.setProgram(tun2SocksExecPath());
    m_t2sProcess.setArguments(args);

    connect(&m_t2sProcess, &QProcess::readyReadStandardOutput, this, [this](){
        qDebug().noquote() << "tun2socks:" << m_t2sProcess.readAllStandardOutput();
    });

    connect(&m_t2sProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus){
        qDebug().noquote() << "ShadowSocksVpnProtocol finished, exitCode, exiStatus" << exitCode << exitStatus;
        setConnectionState(VpnProtocol::Disconnected);
        if (exitStatus != QProcess::NormalExit){
            emit protocolError(amnezia::ErrorCode::ShadowSocksExecutableCrashed);
            stop();
        }
        if (exitCode !=0 ){
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_t2sProcess.start();
    m_t2sProcess.waitForStarted();

    if (m_t2sProcess.state() == QProcess::ProcessState::Running) {

        setConnectionState(VpnConnectionState::Connected);
        return ErrorCode::NoError;
    }
    else return ErrorCode::ShadowSocksExecutableMissing;
#else
    return ErrorCode::NotImplementedError;
#endif

}


void ShadowSocksVpnProtocol::stop()
{
    qDebug() << "ShadowSocksVpnProtocol::stop()";
#ifndef Q_OS_IOS
    m_ssProcess.terminate();
    m_t2sProcess.terminate();
#endif

#ifdef Q_OS_WIN
    Utils::signalCtrl(m_ssProcess.processId(), CTRL_C_EVENT);
#endif
}

QString ShadowSocksVpnProtocol::shadowSocksExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("ss/ss-local"), true);
#else
    return Utils::executable(QString("/ss-local"), true);
#endif
}

QString ShadowSocksVpnProtocol::tun2SocksExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("ss/tun2socks"), true);
#else
    return Utils::executable(QString("/tun2socks"), true);
#endif
}


void ShadowSocksVpnProtocol::readShadowSocksConfiguration(const QJsonObject &configuration)
{
    QJsonObject shadowSocksConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::ShadowSocks)).toObject();
    bool isLocalPortConvertOk = false;
    bool isServerPortConvertOk = false;
    int localPort = shadowSocksConfig.value("local_port").toString().toInt(&isLocalPortConvertOk);
    int serverPort = shadowSocksConfig.value("server_port").toString().toInt(&isServerPortConvertOk);
    if (!isLocalPortConvertOk) {
        qDebug() << "Error when converting local_port field in ShadowSocks config";
    } else if (!isServerPortConvertOk) {
        qDebug() << "Error when converting server_port field in ShadowSocks config";
    }
    shadowSocksConfig["local_port"] = localPort;
    shadowSocksConfig["server_port"] = serverPort;
    shadowSocksConfig["mode"] = "tcp_and_udp";
    m_shadowSocksConfig = shadowSocksConfig;
    m_localPort = localPort;
}
