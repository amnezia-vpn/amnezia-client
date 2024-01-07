#include "xrayprotocol.h"

#include "logger.h"
#include "utilities.h"
#include "containers/containers_defs.h"



#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>

XrayProtocol::XrayProtocol(const QJsonObject &configuration, QObject *parent):
    VpnProtocol(configuration, parent)
{
    readXrayConfiguration(configuration);
    m_routeGateway = Utils::getgatewayandiface();
    m_vpnGateway = "10.33.0.2";
}

XrayProtocol::~XrayProtocol()
{
    XrayProtocol::stop();
    QThread::msleep(200);
#ifndef Q_OS_IOS
    m_xrayProcess.close();
    m_t2sProcess.close();
#endif
}

ErrorCode XrayProtocol::start()
{
    qDebug().noquote() << "XrayProtocol xrayExecPath():" << xrayExecPath();

    if (!QFileInfo::exists(xrayExecPath())) {
        setLastError(ErrorCode::XrayExecutableMissing);
        return lastError();
    }

#ifndef Q_OS_IOS
    if (Utils::processIsRunning(Utils::executable("xray", false))) {
        Utils::killProcessByName(Utils::executable("xray", false));
    }

#ifdef QT_DEBUG
    m_xrayCfgFile.setAutoRemove(false);
#endif
    m_xrayCfgFile.open();
    m_xrayCfgFile.write(QJsonDocument(m_xrayConfig).toJson());
    m_xrayCfgFile.close();

    QStringList args = QStringList() << "-c" << m_xrayCfgFile.fileName() << "-format=json";

    qDebug().noquote() << "XrayProtocol::start()"
                       << xrayExecPath() << args.join(" ");

    m_xrayProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_xrayProcess.setProgram(xrayExecPath());
    m_xrayProcess.setArguments(args);

    connect(&m_xrayProcess, &QProcess::readyReadStandardOutput, this, [this](){
#ifdef QT_DEBUG
        qDebug().noquote() << "xray:" << m_xrayProcess.readAllStandardOutput();
#endif
    });

    connect(&m_xrayProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug().noquote() << "XrayProtocol finished, exitCode, exiStatus" << exitCode << exitStatus;
        setConnectionState(Vpn::ConnectionState::Disconnected);
        if (exitStatus != QProcess::NormalExit) {
            emit protocolError(amnezia::ErrorCode::XrayExecutableCrashed);
            stop();
        }
        if (exitCode != 0) {
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_xrayProcess.start();
    m_xrayProcess.waitForStarted();

    if (m_xrayProcess.state() == QProcess::ProcessState::Running) {
        setConnectionState(Vpn::ConnectionState::Connecting);
        QThread::msleep(1000);
        return startTun2Sock();
    }
    else return ErrorCode::XrayExecutableMissing;
#else
    return ErrorCode::NotImplementedError;
#endif
}


ErrorCode XrayProtocol::startTun2Sock()
{
    if (!QFileInfo::exists(tun2SocksExecPath())) {
        setLastError(ErrorCode::Tun2SockExecutableMissing);
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

    qDebug().noquote() << "XrayProtocol::startTun2Sock()"
                       << tun2SocksExecPath() << args.join(" ");

    m_t2sProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_t2sProcess.setProgram(tun2SocksExecPath());
    m_t2sProcess.setArguments(args);

    connect(&m_t2sProcess, &QProcess::readyReadStandardOutput, this, [this](){
#ifdef QT_DEBUG
        qDebug().noquote() << "tun2socks:" << m_t2sProcess.readAllStandardOutput();
#endif
    });

    connect(&m_t2sProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug().noquote() << "XrayProtocol finished, exitCode, exiStatus" << exitCode << exitStatus;
        setConnectionState(Vpn::ConnectionState::Disconnected);
        if (exitStatus != QProcess::NormalExit){
            emit protocolError(amnezia::ErrorCode::Tun2SockExecutableCrashed);
            stop();
        }
        if (exitCode !=0){
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_t2sProcess.start();
    m_t2sProcess.waitForStarted();

    if (m_t2sProcess.state() == QProcess::ProcessState::Running) {

        setConnectionState(Vpn::ConnectionState::Connected);
        return ErrorCode::NoError;
    }
    else return ErrorCode::Tun2SockExecutableMissing;
#else
    return ErrorCode::NotImplementedError;
#endif
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";
#ifndef Q_OS_IOS
    m_xrayProcess.terminate();
    m_t2sProcess.terminate();
#endif

#ifdef Q_OS_WIN
    Utils::signalCtrl(m_ssProcess.processId(), CTRL_C_EVENT);
#endif
}

QString XrayProtocol::tun2SocksExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("xray/tun2socks"), true);
#else
    return Utils::executable(QString("tun2socks"), true);
#endif
}

QString XrayProtocol::xrayExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("xray/xray"), true);
#else
    return Utils::executable(QString("xray"), true);
#endif
}

void XrayProtocol::readXrayConfiguration(const QJsonObject &configuration)
{
    QJsonObject xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::Xray)).toObject();
    int localPort = 10808;
    m_xrayConfig = xrayConfiguration;
    m_localPort = localPort;
}
