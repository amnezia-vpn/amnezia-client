#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>

#include "logger.h"
#include "openvpnprotocol.h"
#include "utilities.h"
#include "version.h"

OpenVpnProtocol::OpenVpnProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    readOpenVpnConfiguration(configuration);
    connect(&m_managementServer, &ManagementServer::readyRead, this,
            &OpenVpnProtocol::onReadyReadDataFromManagementServer);
}

OpenVpnProtocol::~OpenVpnProtocol()
{
    OpenVpnProtocol::stop();
    QThread::msleep(200);
}

QString OpenVpnProtocol::defaultConfigFileName()
{
    return defaultConfigPath() + QString("/%1.ovpn").arg(APPLICATION_NAME);
}

QString OpenVpnProtocol::defaultConfigPath()
{
    QString p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config";
    Utils::initializePath(p);

    return p;
}

void OpenVpnProtocol::stop()
{
    qDebug() << "OpenVpnProtocol::stop()";
    setConnectionState(Vpn::ConnectionState::Disconnecting);

    // TODO: need refactoring
    // sendTermSignal() will even return true while server connected ???
    if ((m_connectionState == Vpn::ConnectionState::Preparing) || (m_connectionState == Vpn::ConnectionState::Connecting)
        || (m_connectionState == Vpn::ConnectionState::Connected)
        || (m_connectionState == Vpn::ConnectionState::Reconnecting)) {
        if (!sendTermSignal()) {
            killOpenVpnProcess();
        }
        QThread::msleep(10);
        m_managementServer.stop();
    }

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    IpcClient::Interface()->disableKillSwitch();
#endif

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

ErrorCode OpenVpnProtocol::prepare()
{
    if (!IpcClient::Interface()) {
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    QRemoteObjectPendingReply<QStringList> resultCheck = IpcClient::Interface()->getTapList();
    resultCheck.waitForFinished();

    if (resultCheck.returnValue().isEmpty()) {
        QRemoteObjectPendingReply<bool> resultInstall = IpcClient::Interface()->checkAndInstallDriver();
        resultInstall.waitForFinished();

        if (!resultInstall.returnValue())
            return ErrorCode::OpenVpnTapAdapterError;
    }
    return ErrorCode::NoError;
}

void OpenVpnProtocol::killOpenVpnProcess()
{
    if (m_openVpnProcess) {
        m_openVpnProcess->close();
    }
}

void OpenVpnProtocol::readOpenVpnConfiguration(const QJsonObject &configuration)
{
    if (configuration.contains(ProtocolProps::key_proto_config_data(Proto::OpenVpn))) {
        m_configData = configuration;
        QJsonObject jConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::OpenVpn)).toObject();

        m_configFile.open();
        m_configFile.write(jConfig.value(config_key::config).toString().toUtf8());
        m_configFile.close();
        m_configFileName = m_configFile.fileName();
        qDebug().noquote() << QString("Set config data") << m_configFileName;
    }
}

bool OpenVpnProtocol::openVpnProcessIsRunning() const
{
    return Utils::processIsRunning("openvpn");
}

void OpenVpnProtocol::disconnectFromManagementServer()
{
    m_managementServer.stop();
}

QString OpenVpnProtocol::configPath() const
{
    return m_configFileName;
}

void OpenVpnProtocol::sendManagementCommand(const QString &command)
{
    QIODevice *device = dynamic_cast<QIODevice *>(m_managementServer.socket().data());
    if (device) {
        QTextStream stream(device);
        stream << command << Qt::endl;
    }
}

uint OpenVpnProtocol::selectMgmtPort()
{

    for (int i = 0; i < 100; ++i) {
        quint32 port = QRandomGenerator::global()->generate();
        port = (double)(65000 - 15001) * port / UINT32_MAX + 15001;

        QTcpServer s;
        bool ok = s.listen(QHostAddress::LocalHost, port);
        if (ok)
            return port;
    }

    return m_managementPort;
}

void OpenVpnProtocol::updateRouteGateway(QString line)
{
    if (line.contains("net_route_v4_best_gw")) {
        QStringList params = line.split(" ");
        if (params.size() == 6) {
            m_routeGateway = params.at(3);
        }
    } else {
        line = line.split("ROUTE_GATEWAY", Qt::SkipEmptyParts).at(1);
        if (!line.contains("/"))
            return;
        m_routeGateway = line.split("/", Qt::SkipEmptyParts).first();
        m_routeGateway.replace(" ", "");
    }
    qDebug() << "Set VPN route gateway" << m_routeGateway;
}

ErrorCode OpenVpnProtocol::start()
{
    OpenVpnProtocol::stop();

    if (!QFileInfo::exists(Utils::openVpnExecPath())) {
        setLastError(ErrorCode::OpenVpnExecutableMissing);
        return lastError();
    }

    if (!QFileInfo::exists(configPath())) {
        setLastError(ErrorCode::OpenVpnConfigMissing);
        return lastError();
    }

    // Detect default gateway
#ifdef Q_OS_MAC
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    p.start("route",
            QStringList() << "-n"
                          << "get"
                          << "default");
    p.waitForFinished();
    QString s = p.readAll();

    QRegularExpression rx(R"(gateway:\s*(\d+\.\d+\.\d+\.\d+))");
    QRegularExpressionMatch match = rx.match(s);
    if (match.hasMatch()) {
        m_routeGateway = match.captured(1);
        qDebug() << "Set VPN route gateway" << m_routeGateway;
    } else {
        qWarning() << "Unable to set VPN route gateway, output:\n" << s;
    }
#endif

    uint mgmtPort = selectMgmtPort();
    qDebug() << "OpenVpnProtocol::start mgmt port selected:" << mgmtPort;

    if (!m_managementServer.start(m_managementHost, mgmtPort)) {
        setLastError(ErrorCode::OpenVpnManagementServerError);
        return lastError();
    }

    setConnectionState(Vpn::ConnectionState::Connecting);

    m_openVpnProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_openVpnProcess) {
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_openVpnProcess->waitForSource(5000);
    if (!m_openVpnProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }
    m_openVpnProcess->setProgram(PermittedProcess::OpenVPN);
    QStringList arguments({
            "--config", configPath(), "--management", m_managementHost, QString::number(mgmtPort),
            "--management-client" /*, "--log", vpnLogFileNamePath */
    });
    m_openVpnProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");
    connect(m_openVpnProcess.data(), &PrivilegedProcess::errorOccurred,
            [&](QProcess::ProcessError error) { qDebug() << "PrivilegedProcess errorOccurred" << error; });

    connect(m_openVpnProcess.data(), &PrivilegedProcess::stateChanged,
            [&](QProcess::ProcessState newState) { qDebug() << "PrivilegedProcess stateChanged" << newState; });

    connect(m_openVpnProcess.data(), &PrivilegedProcess::finished, this,
            [&]() { setConnectionState(Vpn::ConnectionState::Disconnected); });

    m_openVpnProcess->start();

    return ErrorCode::NoError;
}

bool OpenVpnProtocol::sendTermSignal()
{
    return m_managementServer.writeCommand("signal SIGTERM");
}

void OpenVpnProtocol::sendByteCount()
{
    m_managementServer.writeCommand("bytecount 1");
}

void OpenVpnProtocol::sendInitialData()
{
    m_managementServer.writeCommand("state on");
    m_managementServer.writeCommand("log on");
}

void OpenVpnProtocol::onReadyReadDataFromManagementServer()
{
    for (;;) {
        QString line = m_managementServer.readLine().simplified();

        if (line.isEmpty()) {
            return;
        }

        if (!line.contains(">BYTECOUNT")) {
            qDebug().noquote() << line;
        }

        if (line.contains(">INFO:OpenVPN Management Interface")) {
            sendInitialData();
        } else if (line.startsWith(">STATE")) {
            if (line.contains("CONNECTED,SUCCESS")) {
                sendByteCount();
                stopTimeoutTimer();
                setConnectionState(Vpn::ConnectionState::Connected);
                continue;
            } else if (line.contains("EXITING,SIGTER")) {
                // openVpnStateSigTermHandler();
                setConnectionState(Vpn::ConnectionState::Disconnecting);
                continue;
            } else if (line.contains("RECONNECTING")) {
                setConnectionState(Vpn::ConnectionState::Reconnecting);
                continue;
            }
        }

        if (line.contains("ROUTE_GATEWAY") || line.contains("net_route_v4_best_gw")) {
            updateRouteGateway(line);
        }

        if (line.contains("PUSH: Received control message")) {
            updateVpnGateway(line);
        }

        if (line.contains("FATAL")) {
            if (line.contains("tap-windows6 adapters on this system are currently in use or disabled")) {
                emit protocolError(ErrorCode::OpenVpnAdaptersInUseError);
            } else {
                emit protocolError(ErrorCode::OpenVpnUnknownError);
            }
            return;
        }

        QByteArray data(line.toStdString().c_str());
        if (data.contains(">BYTECOUNT:")) {
            int beg = data.lastIndexOf(">BYTECOUNT:");
            int end = data.indexOf("\n", beg);

            beg += sizeof(">BYTECOUNT:") - 1;
            QList<QByteArray> count = data.mid(beg, end - beg + 1).split(',');

            quint64 r = static_cast<quint64>(count.at(0).trimmed().toULongLong());
            quint64 s = static_cast<quint64>(count.at(1).trimmed().toULongLong());

            setBytesChanged(r, s);
        }
    }
}

void OpenVpnProtocol::updateVpnGateway(const QString &line)
{
    // line looks like
    // PUSH: Received control message: 'PUSH_REPLY,route 10.8.0.1,topology net30,ping 10,ping-restart
    // 120,ifconfig 10.8.0.6 10.8.0.5,peer-id 0,cipher AES-256-GCM'
    QStringList params = line.split(",");
    for (const QString &l : params) {
        if (l.contains("ifconfig")) {
            if (l.split(" ").size() == 3) {
                m_vpnLocalAddress = l.split(" ").at(1);
                m_vpnGateway = l.split(" ").at(2);
#ifdef Q_OS_WIN
                QThread::msleep(300);
                QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
                for (int i = 0; i < netInterfaces.size(); i++) {
                    for (int j=0; j < netInterfaces.at(i).addressEntries().size(); j++)
                    {
                        if (m_vpnLocalAddress == netInterfaces.at(i).addressEntries().at(j).ip().toString()) {
                            IpcClient::Interface()->enableKillSwitch(QJsonObject(), netInterfaces.at(i).index());
                            m_configData.insert("vpnAdapterIndex", netInterfaces.at(i).index());
                            m_configData.insert("vpnGateway", m_vpnGateway);
                            m_configData.insert("vpnServer", m_configData.value(amnezia::config_key::hostName).toString());
                            IpcClient::Interface()->enablePeerTraffic(m_configData);
                        }
                    }
                }
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
                IpcClient::Interface()->enableKillSwitch(m_configData, 0);
#endif
                qDebug() << QString("Set vpn local address %1, gw %2").arg(m_vpnLocalAddress).arg(vpnGateway());
            }
        }
    }
}
