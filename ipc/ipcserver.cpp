#include "ipcserver.h"

#include <QDebug>

#include "logger.h"

IpcServer::IpcServer(QObject *parent) : IpcInterfaceSource(parent)
{
    connect(&m_xtunController, &XtunController::statusChanged, this,
            [this](amnezia::TunnelStatus status) { emit tunnelStatusChanged(static_cast<int>(status)); });
}

void IpcServer::setLogsEnabled(bool enabled)
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::setLogsEnabled";
#endif

    if (enabled) {
        Logger::init(true);
    } else {
        Logger::deInit();
    }
}

void IpcServer::clearLogs()
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::clearLogs";
#endif

    Logger::clearLogs(true);
}

int IpcServer::tunnelUp(const TunnelConfig &config)
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::tunnelUp";
#endif

    return m_xtunController.up(config);
}

bool IpcServer::tunnelDown()
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::tunnelDown";
#endif

    m_xtunController.down();
    return true;
}

bool IpcServer::setKillSwitchMode(int mode)
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::setKillSwitchMode";
#endif

    return m_xtunController.setKillSwitchMode(static_cast<amnezia::KillSwitchMode>(mode));
}

bool IpcServer::addKillSwitchAllowedRanges(const QStringList &ranges)
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::addKillSwitchAllowedRanges";
#endif

    return m_xtunController.addKillSwitchAllowedRanges(ranges);
}

bool IpcServer::localProxyUp(const QString &config)
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::localProxyUp";
#endif

    return m_xtunController.startLocalProxy(config);
}

bool IpcServer::localProxyDown()
{
#ifdef QT_DEBUG
    qDebug() << "IpcServer::localProxyDown";
#endif

    return m_xtunController.stopLocalProxy();
}

bool IpcServer::isLocalProxyRunning()
{
    return m_xtunController.isLocalProxyRunning();
}
