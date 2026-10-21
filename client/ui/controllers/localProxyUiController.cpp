#include "localProxyUiController.h"

LocalProxyUiController::LocalProxyUiController(LocalProxyController *localProxyController, QObject *parent)
    : QObject(parent), m_localProxyController(localProxyController)
{
    connect(m_localProxyController, &LocalProxyController::stateChanged, this, &LocalProxyUiController::stateChanged);
    connect(m_localProxyController, &LocalProxyController::startFailed, this, &LocalProxyUiController::startFailed);
}

bool LocalProxyUiController::isSupported() const
{
    return m_localProxyController->isSupported();
}

bool LocalProxyUiController::isEnabled() const
{
    return m_localProxyController->isEnabled();
}

int LocalProxyUiController::port() const
{
    return m_localProxyController->port();
}

QString LocalProxyUiController::ownerId() const
{
    return m_localProxyController->ownerId();
}

int LocalProxyUiController::activePort() const
{
    return m_localProxyController->activePort();
}

bool LocalProxyUiController::isPortUserDefined() const
{
    return m_localProxyController->isPortUserDefined();
}

bool LocalProxyUiController::setPort(int port)
{
    return m_localProxyController->setPort(port);
}

bool LocalProxyUiController::isPortBusy(int port) const
{
    return m_localProxyController->isPortBusy(port);
}

bool LocalProxyUiController::enable(const QString &ownerId, int port)
{
    return m_localProxyController->enable(ownerId, port);
}

void LocalProxyUiController::disable()
{
    m_localProxyController->disable();
}
