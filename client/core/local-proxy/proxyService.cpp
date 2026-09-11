#include "proxyService.h"

#include "localProxyDefs.h"

ProxyService::ProxyService(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                           QObject *parent)
    : QObject(parent), m_configManager(serversRepository, appSettingsRepository)
{
}

QJsonObject ProxyService::config() const
{
    return m_cachedConfig;
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
        qCWarning(lcLocalProxy) << "Failed to start Xray:" << m_engine.lastError();
        return false;
    }

    m_cachedConfig = configData->parsedConfig;
    qCInfo(lcLocalProxy) << "Xray started";
    emit xrayStatusChanged(true);
    return true;
}

bool ProxyService::stopXray()
{
    const bool wasRunning = m_engine.isRunning();

    if (!m_engine.stop()) {
        qCWarning(lcLocalProxy) << "Failed to stop Xray:" << m_engine.lastError();
        return false;
    }

    m_cachedConfig = QJsonObject();
    if (wasRunning) {
        qCInfo(lcLocalProxy) << "Xray stopped";
        emit xrayStatusChanged(false);
    }
    return true;
}

bool ProxyService::restartXray()
{
    qCInfo(lcLocalProxy) << "Restarting Xray with updated config";

    if (!stopXray()) {
        return false;
    }

    return startXray();
}

bool ProxyService::isXrayRunning() const
{
    return m_engine.isRunning();
}
