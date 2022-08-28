#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QTcpSocket>
#include <QThread>

#include "debug.h"
#include "wireguardprotocol.h"
#include "utils.h"

WireguardProtocol::WireguardProtocol(const QJsonObject &configuration, QObject* parent) :
    VpnProtocol(configuration, parent)
{
    m_configFile.setFileName(QDir::tempPath() + QDir::separator() + serviceName() + ".conf");
    readWireguardConfiguration(configuration);
}

WireguardProtocol::~WireguardProtocol()
{
    WireguardProtocol::stop();
    QThread::msleep(200);
}

void WireguardProtocol::stop()
{
#ifndef Q_OS_IOS
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


    QStringList arguments({"--remove", configPath()});
    m_wireguardStopProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");

    connect(m_wireguardStopProcess.data(), &PrivilegedProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol Stop errorOccurred" << error;
        setConnectionState(VpnConnectionState::Disconnected);
    });

    connect(m_wireguardStopProcess.data(), &PrivilegedProcess::stateChanged, this, [this](QProcess::ProcessState newState) {
        qDebug() << "WireguardProtocol::WireguardProtocol Stop stateChanged" << newState;
    });

    m_wireguardStopProcess->start();
    m_wireguardStopProcess->waitForFinished(10000);

    setConnectionState(VpnProtocol::Disconnected);
#endif

}

void WireguardProtocol::readWireguardConfiguration(const QJsonObject &configuration)
{
    QJsonObject jConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toObject();

    if (!m_configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical() << "Failed to save wireguard config to" << m_configFile.fileName();
        return;
    }

    m_isConfigLoaded = true;

    m_configFile.write(jConfig.value(config_key::config).toString().toUtf8());
    m_configFile.close();
    m_configFileName = m_configFile.fileName();

    qDebug().noquote() << QString("Set config data") << m_configFileName;
    qDebug().noquote() << QString("Set config data") << configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toString().toUtf8();

}

QString WireguardProtocol::configPath() const
{
    return m_configFileName;
}

void WireguardProtocol::updateRouteGateway(QString line)
{
    // TODO: fix for macos
    line = line.split("ROUTE_GATEWAY", QString::SkipEmptyParts).at(1);
    if (!line.contains("/")) return;
    m_routeGateway = line.split("/", QString::SkipEmptyParts).first();
    m_routeGateway.replace(" ", "");
    qDebug() << "Set VPN route gateway" << m_routeGateway;
}

ErrorCode WireguardProtocol::start()
{
#ifndef Q_OS_IOS
    if (!m_isConfigLoaded) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    WireguardProtocol::stop();

    if (!QFileInfo::exists(Utils::wireguardExecPath())) {
        setLastError(ErrorCode::ExecutableMissing);
        return lastError();
    }

    if (!QFileInfo::exists(configPath())) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    setConnectionState(VpnConnectionState::Connecting);

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


    QStringList arguments({"--add", configPath()});
    m_wireguardStartProcess->setArguments(arguments);

    qDebug() << arguments.join(" ");

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol errorOccurred" << error;
        setConnectionState(VpnConnectionState::Disconnected);
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::stateChanged, this, [this](QProcess::ProcessState newState) {
        qDebug() << "WireguardProtocol::WireguardProtocol stateChanged" << newState;
    });

    connect(m_wireguardStartProcess.data(), &PrivilegedProcess::finished, this, [this]() {
        setConnectionState(VpnConnectionState::Connected);
    });

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
        reply.waitForFinished(1000);
        qDebug() << "WireguardProtocol::WireguardProtocol readAllStandardError" << reply.returnValue();
    });

    m_wireguardStartProcess->start();
    m_wireguardStartProcess->waitForFinished(10000);

    return ErrorCode::NoError;
#else
    return ErrorCode::NotImplementedError;
#endif
}

void WireguardProtocol::updateVpnGateway(const QString &line)
{
//    // line looks like
//    // PUSH: Received control message: 'PUSH_REPLY,route 10.8.0.1,topology net30,ping 10,ping-restart 120,ifconfig 10.8.0.6 10.8.0.5,peer-id 0,cipher AES-256-GCM'

//    QStringList params = line.split(",");
//    for (const QString &l : params) {
//        if (l.contains("ifconfig")) {
//            if (l.split(" ").size() == 3) {
//                m_vpnLocalAddress = l.split(" ").at(1);
//                m_vpnGateway = l.split(" ").at(2);

//                qDebug() << QString("Set vpn local address %1, gw %2").arg(m_vpnLocalAddress).arg(vpnGateway());
//            }
//        }
    //    }
}

QString WireguardProtocol::serviceName() const
{
    return "AmneziaVPN.WireGuard0";
}
