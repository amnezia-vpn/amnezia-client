#include "xrayprotocol.h"

#include "utilities.h"
#include "containers/containers_defs.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>


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
    if (Utils::processIsRunning(Utils::executable(xrayExecPath(), true))) {
        Utils::killProcessByName(Utils::executable(xrayExecPath(), true));
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

    connect(&m_xrayProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        qDebug().noquote() << "xray:" << m_xrayProcess.readAllStandardOutput();
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
    if (!QFileInfo::exists(Utils::tun2socksPath())) {
        setLastError(ErrorCode::Tun2SockExecutableMissing);
        return lastError();
    }

#ifndef Q_OS_IOS
    m_t2sProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_t2sProcess) {
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_t2sProcess->waitForSource(1000);
    if (!m_t2sProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    QString XrayConStr = "socks5://127.0.0.1:" + QString::number(m_localPort);

    m_t2sProcess->setProgram(PermittedProcess::Tun2Socks);
#ifdef Q_OS_WIN
    QStringList arguments({"-device", "tun://tun2", "-proxy", SSConStr, "-tun-post-up",
            "netsh interface ip set address name=\"tun2\" static 10.33.0.2 255.255.255.255"
    });
#endif
#ifdef Q_OS_LINUX
    QStringList arguments({"-device", "tun://tun2", "-proxy", XrayConStr});
#endif
#ifdef Q_OS_MAC
    QStringList arguments({"-device", "utun22", "-proxy", SSConStr});
#endif
    m_t2sProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");
    connect(m_t2sProcess.data(), &PrivilegedProcess::errorOccurred,
            [&](QProcess::ProcessError error) { qDebug() << "PrivilegedProcess errorOccurred" << error; });

    connect(m_t2sProcess.data(), &PrivilegedProcess::stateChanged,
            [&](QProcess::ProcessState newState) {
                qDebug() << "PrivilegedProcess stateChanged" << newState;
        if (newState == QProcess::Running)
        {

#ifdef Q_OS_MAC
            QThread::msleep(5000);
            IpcClient::Interface()->createTun("utun22", "10.33.0.2");
#endif
#ifdef Q_OS_WINDOWS
            QThread::msleep(15000);
#endif
#ifdef Q_OS_LINUX
            QThread::msleep(1000);
            IpcClient::Interface()->createTun("tun2", "10.33.0.2");
#endif
            if (m_routeMode == 0) {
                IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "0.0.0.0/1");
                IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "128.0.0.0/1");
                IpcClient::Interface()->routeAddList(m_routeGateway, QStringList() << m_remoteAddress);
            }
            IpcClient::Interface()->StopRoutingIpv6();
        }
    });

    connect(m_t2sProcess.data(), &PrivilegedProcess::finished, this,
            [&]() {
                setConnectionState(Vpn::ConnectionState::Disconnected);
                IpcClient::Interface()->deleteTun("tun2");
                IpcClient::Interface()->StartRoutingIpv6();
                IpcClient::Interface()->clearSavedRoutes();
    });

    m_t2sProcess->start();
    setConnectionState(Vpn::ConnectionState::Connected);


    return ErrorCode::NoError;
#else
    return ErrorCode::NotImplementedError;
#endif
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";
#ifndef Q_OS_IOS
    m_xrayProcess.terminate();
    if (m_t2sProcess) {
        m_t2sProcess->close();
    }
#endif

#ifdef Q_OS_WIN
    Utils::signalCtrl(m_xrayProcess.processId(), CTRL_C_EVENT);
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
    m_remoteAddress = configuration.value(amnezia::config_key::hostName).toString();
    m_routeMode = configuration.value(amnezia::config_key::splitTunnelType).toInt();
}
