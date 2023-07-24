#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QTcpSocket>
#include <QThread>

#include "logger.h"
#include "utilities.h"
#include "wireguardprotocol.h"

#include "mozilla/localsocketcontroller.h"

WireguardProtocol::WireguardProtocol(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
    m_configFile.setFileName(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    writeWireguardConfiguration(configuration);

    // MZ
#if defined(MZ_LINUX)
    // m_impl.reset(new LinuxController());
#elif defined(MZ_MACOS) // || defined(MZ_WINDOWS)
    m_impl.reset(new LocalSocketController());
    connect(m_impl.get(), &ControllerImpl::connected, this,
            [this](const QString &pubkey, const QDateTime &connectionTimestamp) {
                emit connectionStateChanged(Vpn::ConnectionState::Connected);
            });
    connect(m_impl.get(), &ControllerImpl::disconnected, this,
            [this]() { emit connectionStateChanged(Vpn::ConnectionState::Disconnected); });
    m_impl->initialize(nullptr, nullptr);
#endif
}

WireguardProtocol::~WireguardProtocol()
{
    WireguardProtocol::stop();
    QThread::msleep(200);
}

void WireguardProtocol::stop()
{
#ifdef Q_OS_MAC
    stopMzImpl();
    return;
#endif

    if (!QFileInfo::exists(Utils::wireguardExecPath())) {
        qCritical() << "Wireguard executable missing!";
        setLastError(ErrorCode::ExecutableMissing);
        return;
    }

    m_wireguardStopProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_wireguardStopProcess) {
        qCritical() << "IpcProcess replica is not created!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    m_wireguardStopProcess->waitForSource(1000);
    if (!m_wireguardStopProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return;
    }

    m_wireguardStopProcess->setProgram(PermittedProcess::Wireguard);

    m_wireguardStopProcess->setArguments(stopArgs());
    qDebug() << stopArgs().join(" ");

    connect(m_wireguardStopProcess.data(), &PrivilegedProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol Stop errorOccurred" << error;
        setConnectionState(Vpn::ConnectionState::Disconnected);
    });

    connect(m_wireguardStopProcess.data(), &PrivilegedProcess::stateChanged, this,
            [this](QProcess::ProcessState newState) {
                qDebug() << "WireguardProtocol::WireguardProtocol Stop stateChanged" << newState;
            });

#ifdef Q_OS_LINUX
    if (IpcClient::Interface()) {
        QRemoteObjectPendingReply<bool> result = IpcClient::Interface()->isWireguardRunning();
        if (result.returnValue()) {
            setConnectionState(Vpn::ConnectionState::Disconnected);
            return;
        }
    } else {
        qCritical() << "IPC client not initialized";
        setConnectionState(Vpn::ConnectionState::Disconnected);
        return;
    }
#endif

    m_wireguardStopProcess->start();
    m_wireguardStopProcess->waitForFinished(10000);

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

#ifdef Q_OS_MAC
ErrorCode WireguardProtocol::startMzImpl()
{
    m_impl->activate(m_rawConfig);
    return ErrorCode::NoError;
}

ErrorCode WireguardProtocol::stopMzImpl()
{
    m_impl->deactivate();
    return ErrorCode::NoError;
}
#endif

void WireguardProtocol::writeWireguardConfiguration(const QJsonObject &configuration)
{
    QJsonObject jConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toObject();

    if (!m_configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical() << "Failed to save wireguard config to" << m_configFile.fileName();
        return;
    }

    m_configFile.write(jConfig.value(config_key::config).toString().toUtf8());
    m_configFile.close();

#ifdef Q_OS_LINUX
    if (IpcClient::Interface()) {
        QRemoteObjectPendingReply<bool> result = IpcClient::Interface()->copyWireguardConfig(m_configFile.fileName());
        if (result.returnValue()) {
            qCritical() << "Failed to copy wireguard config";
            return;
        }
    } else {
        qCritical() << "IPC client not initialized";
        return;
    }
    m_configFileName = "/etc/wireguard/wg99.conf";
#else
    m_configFileName = m_configFile.fileName();
#endif

    m_isConfigLoaded = true;

    qDebug().noquote() << QString("Set config data") << configPath();
    qDebug().noquote() << QString("Set config data")
                       << configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toString().toUtf8();
}

QString WireguardProtocol::configPath() const
{
    return m_configFileName;
}

void WireguardProtocol::updateRouteGateway(QString line)
{
    // TODO: fix for macos
    line = line.split("ROUTE_GATEWAY", Qt::SkipEmptyParts).at(1);
    if (!line.contains("/"))
        return;
    m_routeGateway = line.split("/", Qt::SkipEmptyParts).first();
    m_routeGateway.replace(" ", "");
    qDebug() << "Set VPN route gateway" << m_routeGateway;
}

ErrorCode WireguardProtocol::start()
{
    if (!m_isConfigLoaded) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

#ifdef Q_OS_MAC
    return startMzImpl();
#endif

    if (!QFileInfo::exists(Utils::wireguardExecPath())) {
        setLastError(ErrorCode::ExecutableMissing);
        return lastError();
    }

    if (IpcClient::Interface()) {
        QRemoteObjectPendingReply<bool> result = IpcClient::Interface()->isWireguardConfigExists(configPath());
        if (result.returnValue()) {
            setLastError(ErrorCode::ConfigMissing);
            return lastError();
        }
    } else {
        qCritical() << "IPC client not initialized";
        setLastError(ErrorCode::InternalError);
        return lastError();
    }

    setConnectionState(Vpn::ConnectionState::Connecting);

    m_wireguardStartProcess = IpcClient::CreatePrivilegedProcess();

    if (!m_wireguardStartProcess) {
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_wireguardStartProcess->waitForSource(1000);
    if (!m_wireguardStartProcess->isInitialized()) {
        qWarning() << "IpcProcess replica is not connected!";
        setLastError(ErrorCode::AmneziaServiceConnectionFailed);
        return ErrorCode::AmneziaServiceConnectionFailed;
    }

    m_wireguardStartProcess->setProgram(PermittedProcess::Wireguard);

    m_wireguardStartProcess->setArguments(startArgs());
    qDebug() << startArgs().join(" ");

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol errorOccurred" << error;
        setConnectionState(Vpn::ConnectionState::Disconnected);
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::stateChanged, this,
            [this](QProcess::ProcessState newState) {
                qDebug() << "WireguardProtocol::WireguardProtocol stateChanged" << newState;
            });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::finished, this,
            [this]() { setConnectionState(Vpn::ConnectionState::Connected); });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::readyRead, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAll();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readyRead" << reply.returnValue();
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::readyReadStandardOutput, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAllStandardOutput();
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardOutput" << reply.returnValue();
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::readyReadStandardError, this, [this]() {
        QRemoteObjectPendingReply<QByteArray> reply = m_wireguardStartProcess->readAllStandardError();
        reply.waitForFinished(10);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardError" << reply.returnValue();
    });

    m_wireguardStartProcess->start();
    m_wireguardStartProcess->waitForFinished(10000);

    return ErrorCode::NoError;
}

void WireguardProtocol::updateVpnGateway(const QString &line)
{
}

QString WireguardProtocol::serviceName() const
{
    return "AmneziaVPN.WireGuard0";
}

QStringList WireguardProtocol::stopArgs()
{
#ifdef Q_OS_WIN
    return { "--remove", configPath() };
#elif defined Q_OS_LINUX
    return { "down", "wg99" };
#else
    return {};
#endif
}

QStringList WireguardProtocol::startArgs()
{
#ifdef Q_OS_WIN
    return { "--add", configPath() };
#elif defined Q_OS_LINUX
    return { "up", "wg99" };
#else
    return {};
#endif
}
