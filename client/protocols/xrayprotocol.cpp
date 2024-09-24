#include "xrayprotocol.h"

#include "utilities.h"
#include "containers/containers_defs.h"
#include "core/networkUtilities.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>


XrayProtocol::XrayProtocol(const QJsonObject &configuration, QObject *parent):
    VpnProtocol(configuration, parent)
{
    readXrayConfiguration(configuration);
    m_routeGateway = NetworkUtilities::getGatewayAndIface();
    m_vpnGateway = amnezia::protocols::xray::defaultLocalAddr;
    m_vpnLocalAddress = amnezia::protocols::xray::defaultLocalAddr;
    m_t2sProcess = IpcClient::InterfaceTun2Socks();
}

XrayProtocol::~XrayProtocol()
{
    XrayProtocol::stop();
    QThread::msleep(200);
    m_xrayProcess.close();
}

ErrorCode XrayProtocol::start()
{
    qDebug().noquote() << "XrayProtocol xrayExecPath():" << xrayExecPath();

    if (!QFileInfo::exists(xrayExecPath())) {
        setLastError(ErrorCode::XrayExecutableMissing);
        return lastError();
    }

    if (Utils::processIsRunning(Utils::executable(xrayExecPath(), true))) {
        Utils::killProcessByName(Utils::executable(xrayExecPath(), true));
    }

#ifdef QT_DEBUG
    m_xrayCfgFile.setAutoRemove(false);
#endif
    m_xrayCfgFile.open();
    QString config = QJsonDocument(m_xrayConfig).toJson();
    config.replace(m_remoteHost, m_remoteAddress);
    m_xrayCfgFile.write(config.toUtf8());
    m_xrayCfgFile.close();

    QStringList args = QStringList() << "-c" << m_xrayCfgFile.fileName() << "-format=json";

    qDebug().noquote() << "XrayProtocol::start()"
                       << xrayExecPath() << args.join(" ");

    m_xrayProcess.setProcessChannelMode(QProcess::MergedChannels);

    m_xrayProcess.setProgram(xrayExecPath());
    m_xrayProcess.setArguments(args);

    connect(&m_xrayProcess, &QProcess::readyReadStandardOutput, this, [this]() {
#ifdef QT_DEBUG
        qDebug().noquote() << "xray:" << m_xrayProcess.readAllStandardOutput();
#endif
    });

    connect(&m_xrayProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug().noquote() << "XrayProtocol finished, exitCode, exitStatus" << exitCode << exitStatus;
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
}


ErrorCode XrayProtocol::startTun2Sock()
{
    m_t2sProcess->start();

#ifdef Q_OS_WIN
    m_configData.insert("inetAdapterIndex", NetworkUtilities::AdapterIndexTo(QHostAddress(m_remoteAddress)));
#endif

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::stateChanged, this,
            [&](QProcess::ProcessState newState) { qDebug() << "PrivilegedProcess stateChanged" << newState; });

    connect(m_t2sProcess.data(), &IpcProcessTun2SocksReplica::setConnectionState, this,
            [&](int vpnState) {
                qDebug() << "PrivilegedProcess setConnectionState " << vpnState;
                if (vpnState == Vpn::ConnectionState::Connected)
                {
                    setConnectionState(Vpn::ConnectionState::Connecting);
                    QList<QHostAddress> dnsAddr;
                    dnsAddr.push_back(QHostAddress(m_configData.value(config_key::dns1).toString()));
                    dnsAddr.push_back(QHostAddress(m_configData.value(config_key::dns2).toString()));
#ifdef Q_OS_WIN
                    QThread::msleep(8000);
#endif
#ifdef Q_OS_MACOS
                    QThread::msleep(5000);
                    IpcClient::Interface()->createTun("utun22", amnezia::protocols::xray::defaultLocalAddr);
                    IpcClient::Interface()->updateResolvers("utun22", dnsAddr);
#endif
#ifdef Q_OS_LINUX
                    QThread::msleep(1000);
                    IpcClient::Interface()->createTun("tun2", amnezia::protocols::xray::defaultLocalAddr);
                    IpcClient::Interface()->updateResolvers("tun2", dnsAddr);
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
                    // killSwitch toggle
                    if (QVariant(m_configData.value(config_key::killSwitchOption).toString()).toBool()) {
                        m_configData.insert("vpnServer", m_remoteAddress);
                        IpcClient::Interface()->enableKillSwitch(m_configData, 0);
                    }
#endif
                    if (m_routeMode == 0) {
                        IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "0.0.0.0/1");
                        IpcClient::Interface()->routeAddList(m_vpnGateway, QStringList() << "128.0.0.0/1");
                        IpcClient::Interface()->routeAddList(m_routeGateway, QStringList() << m_remoteAddress);
                    }
                    IpcClient::Interface()->StopRoutingIpv6();
#ifdef Q_OS_WIN
                    IpcClient::Interface()->updateResolvers("tun2", dnsAddr);
                    QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
                    for (int i = 0; i < netInterfaces.size(); i++) {
                        for (int j = 0; j < netInterfaces.at(i).addressEntries().size(); j++)
                        {
                            // killSwitch toggle
                            if (m_vpnLocalAddress == netInterfaces.at(i).addressEntries().at(j).ip().toString()) {
                                if (QVariant(m_configData.value(config_key::killSwitchOption).toString()).toBool()) {
                                    IpcClient::Interface()->enableKillSwitch(QJsonObject(), netInterfaces.at(i).index());
                                }
                                m_configData.insert("vpnAdapterIndex", netInterfaces.at(i).index());
                                m_configData.insert("vpnGateway", m_vpnGateway);
                                m_configData.insert("vpnServer", m_remoteAddress);
                                IpcClient::Interface()->enablePeerTraffic(m_configData);
                            }
                        }
                    }
#endif
                    setConnectionState(Vpn::ConnectionState::Connected);
                }
#if !defined(Q_OS_MACOS)
                if (vpnState == Vpn::ConnectionState::Disconnected) {
                    setConnectionState(Vpn::ConnectionState::Disconnected);
                    IpcClient::Interface()->deleteTun("tun2");
                    IpcClient::Interface()->StartRoutingIpv6();
                    IpcClient::Interface()->clearSavedRoutes();
                }
#endif
            });

    return ErrorCode::NoError;
}

void XrayProtocol::stop()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    IpcClient::Interface()->disableKillSwitch();
    IpcClient::Interface()->StartRoutingIpv6();
#endif
    qDebug() << "XrayProtocol::stop()";
    m_xrayProcess.terminate();
    if (m_t2sProcess) {
        m_t2sProcess->stop();
    }

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
    m_configData = configuration;
    QJsonObject xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::Xray)).toObject();
    if (xrayConfiguration.isEmpty()) {
        xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::SSXray)).toObject();
    }
    m_xrayConfig = xrayConfiguration;
    m_localPort = QString(amnezia::protocols::xray::defaultLocalProxyPort).toInt();
    m_remoteHost = configuration.value(amnezia::config_key::hostName).toString();
    m_remoteAddress = NetworkUtilities::getIPAddress(m_remoteHost);
    m_routeMode = configuration.value(amnezia::config_key::splitTunnelType).toInt();
    m_primaryDNS = configuration.value(amnezia::config_key::dns1).toString();
    m_secondaryDNS = configuration.value(amnezia::config_key::dns2).toString();
}
