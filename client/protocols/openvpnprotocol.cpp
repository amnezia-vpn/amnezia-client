#include <QCoreApplication>
#include <QFileInfo>
#include <QTcpSocket>

#include "communicator.h"
#include "debug.h"
#include "openvpnprotocol.h"
#include "utils.h"


OpenVpnProtocol::OpenVpnProtocol(const QString& args, QObject* parent) :
    VpnProtocol(args, parent),
    m_requestFromUserToStop(false)
{
    setConfigFile(args);
    connect(m_communicator, &Communicator::messageReceived, this, &OpenVpnProtocol::onMessageReceived);
    connect(&m_managementServer, &ManagementServer::readyRead, this, &OpenVpnProtocol::onReadyReadDataFromManagementServer);
}

OpenVpnProtocol::~OpenVpnProtocol()
{
    stop();
}

void OpenVpnProtocol::onMessageReceived(const Message& message)
{
    if (!message.isValid()) {
        qWarning().noquote() << QString("Message received: '%1', but it is not valid").arg(message.toString());
        return;
    }

    switch (message.state()) {
    case Message::State::Started:
        qDebug() << "OpenVPN process started";
        break;
    case Message::State::Finished:
        qDebug().noquote() << QString("OpenVPN process finished with status %1").arg(message.argAtIndex(1));
        onOpenVpnProcessFinished(message.argAtIndex(1).toInt());
        break;
    default:
        qDebug().noquote() << QString("Message received: '%1'").arg(message.toString());
        ;
    }
}

void OpenVpnProtocol::stop()
{
    // TODO: need refactoring
    // sendTermSignal() will evet return true while server connected
    if ((m_connectionState == VpnProtocol::ConnectionState::Preparing) ||
            (m_connectionState == VpnProtocol::ConnectionState::Connecting) ||
            (m_connectionState == VpnProtocol::ConnectionState::Connected) ||
            (m_connectionState == VpnProtocol::ConnectionState::TunnelReconnecting)) {
        if (!sendTermSignal()) {
            killOpenVpnProcess();
        }
        setConnectionState(VpnProtocol::ConnectionState::Disconnecting);
    }
}

void OpenVpnProtocol::killOpenVpnProcess()
{
    // send command to kill openvpn process (if any).
}

bool OpenVpnProtocol::setConfigFile(const QString& configFileNamePath)
{
    m_configFileName = configFileNamePath;
    QFileInfo file(m_configFileName);

    if (file.fileName().isEmpty()) {
        m_configFileName = Utils::defaultVpnConfigFileName();
    }

    if (m_configFileName.isEmpty()) {
        return false;
    }

    qDebug().noquote() << QString("Set config file: '%1'").arg(configPath());

    return false;
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

void OpenVpnProtocol::writeCommand(const QString& command)
{
    QIODevice *device = dynamic_cast<QIODevice*>(m_managementServer.socket().data());
    if (device) {
        QTextStream stream(device);
        stream << command << endl;
    }
}

QString OpenVpnProtocol::openVpnExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("openvpn/%1/openvpn").arg(QSysInfo::buildCpuArchitecture()), true);
#else
    return Utils::executable(QString("/openvpn"), true);
#endif
}

ErrorCode OpenVpnProtocol::start()
{
    qDebug() << "Start OpenVPN connection";

    m_requestFromUserToStop = false;
    m_openVpnStateSigTermHandlerTimer.stop();
    stop();

    if (communicator() && !communicator()->isConnected()) {
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return lastError();
    }

    if (!QFileInfo::exists(openVpnExecPath())) {
        setLastError(ErrorCode::OpenVpnExecutableMissing);
        return lastError();
    }

    if (!QFileInfo::exists(configPath())) {
        setLastError(ErrorCode::OpenVpnConfigMissing);
        return lastError();
    }

    QString vpnLogFileNamePath = Utils::systemLogPath() + "/openvpn.log";
    Utils::createEmptyFile(vpnLogFileNamePath);

    QStringList args({openVpnExecPath(),
                      "--config" , configPath(),
                      "--management", m_managementHost, QString::number(m_managementPort),
                      "--management-client",
                      "--log-append", vpnLogFileNamePath
                     });

    if (!m_managementServer.start(m_managementHost, m_managementPort)) {
        setLastError(ErrorCode::OpenVpnManagementServerError);
        return lastError();
    }

    setConnectionState(ConnectionState::Connecting);
    m_communicator->sendMessage(Message(Message::State::StartRequest, args));
    startTimeoutTimer();

    return ErrorCode::NoError;
}

void OpenVpnProtocol::openVpnStateSigTermHandlerTimerEvent()
{
    bool processStatus = openVpnProcessIsRunning();
    if (processStatus) {
        killOpenVpnProcess();
    }
    onOpenVpnProcessFinished(0);
}

void OpenVpnProtocol::openVpnStateSigTermHandler()
{
    m_openVpnStateSigTermHandlerTimer.start(5000);
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
    for (;;)  {
        QString line = m_managementServer.readLine().simplified();

        if (line.isEmpty()) {
            return;
        }

        if (!line.contains(">BYTECOUNT")) {
            qDebug().noquote() << line;
        }

        if (line.contains(">INFO:OpenVPN Management Interface")) {
            sendInitialData();
        }  else if (line.startsWith(">STATE")) {
            if (line.contains("CONNECTED,SUCCESS")) {
                sendByteCount();
                stopTimeoutTimer();
                setConnectionState(VpnProtocol::ConnectionState::Connected);
                continue;
            } else if (line.contains("EXITING,SIGTER")) {
                openVpnStateSigTermHandler();
                continue;
            } else if (line.contains("RECONNECTING")) {
                setConnectionState(VpnProtocol::ConnectionState::TunnelReconnecting);
                continue;
            }
        }

        if (line.contains("FATAL")) {
            if (line.contains("tap-windows6 adapters on this system are currently in use or disabled")) {
                emit protocolError(ErrorCode::OpenVpnAdaptersInUseError);
            }
            else {
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

void OpenVpnProtocol::onOpenVpnProcessFinished(int exitCode)
{
    m_openVpnStateSigTermHandlerTimer.stop();
    if (m_connectionState == VpnProtocol::ConnectionState::Disconnected) {
        qDebug() << "Already in disconnected state";
        return;
    }

    qDebug().noquote() << QString("Process finished with code: %1").arg(exitCode);

    setConnectionState(VpnProtocol::ConnectionState::Disconnected);
}


