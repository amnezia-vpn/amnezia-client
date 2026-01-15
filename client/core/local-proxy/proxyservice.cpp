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

ProxyService::ProxyService(const std::shared_ptr<Settings> &settings, QObject* parent)
    : QObject(parent)
    , m_configManager(new ConfigManager(settings))
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

    QString error;
    const auto configData = m_configManager->buildConfig(error);
    if (!configData) {
        logConfigError(error);
        return false;
    }

    m_cachedConfig = configData->parsedConfig;

    const bool success = m_xrayController->start(configData->serializedConfig);
    if (success) {
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