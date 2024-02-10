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


ErrorCode WireguardProtocol::start()
{
    return startMzImpl();
}

