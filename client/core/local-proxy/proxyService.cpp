#include "proxyService.h"

#include <QDebug>


ProxyService::ProxyService(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                           QObject *parent)
    : QObject(parent), m_configManager(serversRepository, appSettingsRepository)
{
}

bool ProxyService::startXray()
{
    if (m_engine.isRunning()) {
        return true;
    }

    QString error;
    const auto configData = m_configManager.buildConfig(error);
    if (!configData) {
        return false;
    }

    if (!m_engine.start(configData->serializedConfig)) {
        qWarning() << "Failed to start Xray:" << m_engine.lastError();
        return false;
    }

    m_activePort = configData->proxyPort;
    qDebug() << "Xray started on port" << m_activePort;
    emit xrayStatusChanged(true, m_activePort);
    return true;
}

bool ProxyService::stopXray()
{
    const bool wasRunning = m_engine.isRunning();

    if (!m_engine.stop()) {
        qWarning() << "Failed to stop Xray:" << m_engine.lastError();
        return false;
    }

    m_activePort = 0;
    if (wasRunning) {
        qDebug() << "Xray stopped";
        emit xrayStatusChanged(false, 0);
    }
    return true;
}

bool ProxyService::restartXray()
{
    qDebug() << "Restarting Xray with updated config";

    if (!stopXray()) {
        return false;
    }

    return startXray();
}

bool ProxyService::isXrayRunning() const
{
    return m_engine.isRunning();
}

int ProxyService::activePort() const
{
    return m_engine.isRunning() ? m_activePort : 0;
}
