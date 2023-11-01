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

    m_impl.reset(new LocalSocketController());
    connect(m_impl.get(), &ControllerImpl::connected, this,
            [this](const QString &pubkey, const QDateTime &connectionTimestamp) {
                emit connectionStateChanged(Vpn::ConnectionState::Connected);
            });
    connect(m_impl.get(), &ControllerImpl::disconnected, this,
            [this]() { emit connectionStateChanged(Vpn::ConnectionState::Disconnected); });
    m_impl->initialize(nullptr, nullptr);
}

WireguardProtocol::~WireguardProtocol()
{
    WireguardProtocol::stop();
    QThread::msleep(200);
}

void WireguardProtocol::stop()
{
    stopMzImpl();
    return;
}

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

void WireguardProtocol::writeWireguardConfiguration(const QJsonObject &configuration)
{
    QJsonObject jConfig = configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toObject();

    if (!m_configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical() << "Failed to save wireguard config to" << m_configFile.fileName();
        return;
    }

    m_configFile.write(jConfig.value(config_key::config).toString().toUtf8());
    m_configFile.close();


    m_configFileName = m_configFile.fileName();

    m_isConfigLoaded = true;

    qDebug().noquote() << QString("Set config data") << configPath();
    qDebug().noquote() << QString("Set config data")
                       << configuration.value(ProtocolProps::key_proto_config_data(Proto::WireGuard)).toString().toUtf8();
}

QString WireguardProtocol::configPath() const
{
    return m_configFileName;
}

QString WireguardProtocol::serviceName() const
{
    return "AmneziaVPN.WireGuard0";
}

ErrorCode WireguardProtocol::start()
{
    if (!m_isConfigLoaded) {
        setLastError(ErrorCode::ConfigMissing);
        return lastError();
    }

    return startMzImpl();
}

