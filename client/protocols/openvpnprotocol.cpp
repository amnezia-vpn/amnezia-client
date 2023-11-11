#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>

#include "logger.h"
#include "openvpnprotocol.h"
#include "utilities.h"
#include "version.h"


OpenVpnProtocol::OpenVpnProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    readOpenVpnConfiguration(configuration);
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

    // TODO: need refactoring
    // sendTermSignal() will even return true while server connected ???
    if ((m_connectionState == Vpn::ConnectionState::Preparing) || (m_connectionState == Vpn::ConnectionState::Connecting)
        || (m_connectionState == Vpn::ConnectionState::Connected)
        || (m_connectionState == Vpn::ConnectionState::Reconnecting)) {
            killOpenVpnProcess();
            QThread::msleep(10);
    }
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
        QJsonObject jConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::OpenVpn)).toObject();
        QString plainConfig = jConfig.value(config_key::config).toString().toUtf8();
        if (configuration.contains(ProtocolProps::key_proto_config_data(Proto::Cloak))) {
            QJsonObject cloakConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::Cloak)).toObject();
            cloakConfig["NumConn"] = 1;
            cloakConfig["ProxyMethod"] = "openvpn";
            if (cloakConfig.contains("port")) {
                int portValue = cloakConfig.value("port").toInt();
                cloakConfig.remove("port");
                cloakConfig["RemotePort"] = portValue;
            }
            if (cloakConfig.contains("remote")) {
                QString hostValue = cloakConfig.value("remote").toString();
                cloakConfig.remove("remote");
                cloakConfig["RemoteHost"] = hostValue;
            }
            plainConfig += "\n<cloak>\n";
            QJsonDocument Doc(cloakConfig);
            QByteArray ba = Doc.toJson();
            QString plainCloak = ba;
            plainConfig += QString::fromLatin1(plainCloak.toUtf8().toBase64().data());
            plainConfig += "\n</cloak>\n";
        }
        m_configFile.open();
        m_configFile.write(plainConfig.toUtf8());
        m_configFile.close();
        m_configFileName = m_configFile.fileName();

        qDebug().noquote() << QString("Set config data") << m_configFileName;
    }
}

bool OpenVpnProtocol::openVpnProcessIsRunning() const
{
    return Utils::processIsRunning("ovpncli");
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
    const QString substr = "sitnl_route_best_gw result: via ";
    int start = line.indexOf(substr) + substr.size();
    int end = line.indexOf(" dev ", start);

    m_routeGateway =  line.mid(start, (end-start));
}

void OpenVpnProtocol::handle_cli_message(QString message)
{
    QString line = message;

    if (line.isEmpty()) {
        return;
    }

    if (line.contains("EVENT: CONNECTED")) {
        setConnectionState(Vpn::ConnectionState::Connected);
    } else if (line.contains("EXITING")) {
        // openVpnStateSigTermHandler();
        setConnectionState(Vpn::ConnectionState::Disconnecting);
    } else if (line.contains("RECONNECTING")) {
        setConnectionState(Vpn::ConnectionState::Reconnecting);
    }

    if (line.contains("sitnl_route_best_gw")) {
        updateRouteGateway(line);
    }

    if (line.contains("[ifconfig]")) {
        updateVpnGateway(line);
    }

    // TODO: SET CORRECT STRING
    if (line.contains("FATAL")) {
        if (line.contains("tap-windows6 adapters on this system are currently in use or disabled")) {
            emit protocolError(ErrorCode::OpenVpnAdaptersInUseError);
        } else {
            emit protocolError(ErrorCode::OpenVpnUnknownError);
        }
        return;
    }

}


ErrorCode OpenVpnProtocol::start()
{
    // qDebug() << "Start OpenVPN connection";
    OpenVpnProtocol::stop();

    qDebug() << " Utils::openVpnExecPath();" << Utils::openVpnExecPath();

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

    setConnectionState(Vpn::ConnectionState::Connecting);
    m_openVpnProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_openVpnProcess) {
        qWarning() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_openVpnProcess->waitForSource(1000);
    if (!m_openVpnProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }
    m_openVpnProcess->setProgram(PermittedProcess::OpenVPN);
    QStringList arguments({ configPath()/*, "--management", m_managementHost, QString::number(mgmtPort),
            "--management-client" *//*, "--log", vpnLogFileNamePath */
    });
    m_openVpnProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");
    connect(m_openVpnProcess.data(), &PrivilegedProcess::errorOccurred,
            [&](QProcess::ProcessError error) {
                qDebug() << "PrivilegedProcess errorOccurred" << error;
                setConnectionState(Vpn::ConnectionState::Disconnected);
    });

    connect(m_openVpnProcess.data(), &PrivilegedProcess::stateChanged, [&](QProcess::ProcessState newState) {
        switch ( newState )
        {
            case QProcess::Starting:
                setConnectionState(Vpn::ConnectionState::Connecting);
            break;
            case QProcess::Running:
                setConnectionState(Vpn::ConnectionState::Connecting);
            break;
            default:
                setConnectionState(Vpn::ConnectionState::Disconnected);
        }
        qDebug() << "PrivilegedProcess stateChanged" << newState;
    });

    connect(m_openVpnProcess.data(), &PrivilegedProcess::finished, this,
            [&]() { setConnectionState(Vpn::ConnectionState::Disconnected); });


    connect(m_openVpnProcess.data(), &PrivilegedProcess::readyRead, this, [&] {

        QRemoteObjectPendingReply<QByteArray> call = m_openVpnProcess->readAll();
        auto *watcher = new QRemoteObjectPendingCallWatcher(call, this);

        auto *timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);
        m_watchers.insert(watcher, timeoutTimer);

        connect(timeoutTimer, &QTimer::timeout, this, [this, watcher, timeoutTimer]() {
            qDebug() << "Foo request timed out.";

            m_watchers.remove(watcher);
            watcher->deleteLater();
            timeoutTimer->deleteLater();
        });

        connect(watcher, &QRemoteObjectPendingCallWatcher::finished, [this](QRemoteObjectPendingCallWatcher *self) {
            QTimer *timer = m_watchers.take(self);
            if (timer) {
                timer->stop();
                timer->deleteLater();
            }

            QByteArray result = self->returnValue().toByteArray();
            handle_cli_message(QString(result));
            self->deleteLater();
        });

        timeoutTimer->start(30000);

    });

    connect(m_openVpnProcess.data(), QOverload<int, QProcess::ExitStatus>::of(&PrivilegedProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug().noquote() << "OpenVPN finished, exitCode, exiStatus" << exitCode << exitStatus;
        setConnectionState(Vpn::ConnectionState::Disconnected);
        if (exitStatus != QProcess::NormalExit) {
            emit protocolError(amnezia::ErrorCode::ShadowSocksExecutableCrashed);
            stop();
        }
        if (exitCode !=0 ) {
            emit protocolError(amnezia::ErrorCode::InternalError);
            stop();
        }
    });

    m_openVpnProcess->start();

    return ErrorCode::NoError;
}

void OpenVpnProtocol::updateVpnGateway(const QString &line)
{
    // "[ifconfig] [10.8.0.14] [10.8.0.13]"
    QStringList params = line.split("\n");
    for (const QString &param : params) {
        if (param.contains("ifconfig")) {
            QString l = param.right(param.size() - param.indexOf("ifconfig"));
            if (l.split(" ").size() == 3) {
                m_vpnLocalAddress = l.split(" ").at(1);
                m_vpnLocalAddress.remove("[");m_vpnLocalAddress.remove("]");
                m_vpnGateway = l.split(" ").at(2);
                m_vpnGateway.remove("[");m_vpnGateway.remove("]");
                qDebug() << QString("Set vpn local address %1, gw %2").arg(m_vpnLocalAddress).arg(vpnGateway());
            }
        }
    }
}
