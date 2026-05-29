#include "proxyservice.h"

#include "proxylogger.h"

namespace {

void logConfigError(const QString &errorMessage)
{
    if (!errorMessage.isEmpty()) {
        ProxyLogger::getInstance().error(errorMessage);
    }
}

} // namespace

ProxyService::ProxyService(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository,
                           QObject *parent)
    : QObject(parent)
    , m_configManager(new ConfigManager(serversRepository, appSettingsRepository))
    , m_xrayController(new XrayController())
{
    ProxyLogger::getInstance().debug("ProxyService initialized");
}

QJsonObject ProxyService::getConfig()
{
    if (!m_cachedConfig.isEmpty()) {
        return m_cachedConfig;
    }

    QString error;
    const auto configData = m_configManager->buildConfigWithFetch(error);
    if (!configData) {
        logConfigError(error);
        return {};
    }

    m_cachedConfig = configData->parsedConfig;
    return m_cachedConfig;
}

bool ProxyService::startXray()
{
    ProxyLogger::getInstance().info("Starting Xray");

    if (m_xrayController->isXrayRunning()) {
        ProxyLogger::getInstance().info("Xray is already running");
        return true;
    }

    QString error;
    const auto configData = m_configManager->buildConfigWithFetch(error);
    if (!configData) {
        logConfigError(error);
        return false;
    }

    const bool success = m_xrayController->start(configData->serializedConfig);
    if (success) {
        m_cachedConfig = configData->parsedConfig;
        ProxyLogger::getInstance().info("Xray started successfully");
        emit xrayStatusChanged(true);
        return true;
    }

    ProxyLogger::getInstance().error(QStringLiteral("Failed to start Xray: %1").arg(m_xrayController->getError()));
    return false;
}

bool ProxyService::stopXray()
{
    ProxyLogger::getInstance().info("Stopping Xray");
    const bool stopped = m_xrayController->stop();
    if (stopped) {
        ProxyLogger::getInstance().info("Xray stopped");
        emit xrayStatusChanged(false);
        return true;
    }

    ProxyLogger::getInstance().warning(QStringLiteral("Failed to stop Xray: %1").arg(m_xrayController->getError()));
    return false;
}

bool ProxyService::isXrayRunning() const
{
    return m_xrayController->isXrayRunning();
}

qint64 ProxyService::getXrayProcessId() const
{
    return m_xrayController->getProcessId();
}

QString ProxyService::getXrayError() const
{
    return m_xrayController->getError();
}

void ProxyService::clearCache()
{
    m_cachedConfig = QJsonObject();
    ProxyLogger::getInstance().debug("ProxyService cache cleared");
}

bool ProxyService::restartXray()
{
    ProxyLogger::getInstance().info("Restarting Xray with updated config");
    clearCache();

    if (m_xrayController->isXrayRunning()) {
        if (!stopXray()) {
            ProxyLogger::getInstance().error("Failed to stop Xray during restart, aborting");
            return false;
        }
    }

    return startXray();
}