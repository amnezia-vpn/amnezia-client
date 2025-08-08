#include "proxyservice.h"
#include "proxylogger.h"

ProxyService::ProxyService(QObject* parent)
    : QObject(parent)
    , m_configManager(new ConfigManager())
    , m_xrayController(new XrayController())
{
    ProxyLogger::getInstance().debug("ProxyService initialized");
}

QJsonObject ProxyService::getConfig() const
{
    ProxyLogger::getInstance().debug("Getting active config");
    return m_configManager->getActiveConfig();
}

bool ProxyService::updateConfig(const QString& configStr)
{
    ProxyLogger::getInstance().info("Updating config");
    bool success = m_configManager->updateAllConfigs({configStr});
    if (success) {
        ProxyLogger::getInstance().info("Config updated successfully");
        emit configsChanged();
    } else {
        ProxyLogger::getInstance().error("Failed to update config");
    }
    return success;
}

bool ProxyService::startXray()
{
    ProxyLogger::getInstance().info("Starting Xray");
    auto activeConfig = m_configManager->getActiveConfigPath();
    qDebug() << activeConfig;
    bool success = m_xrayController->start(m_configManager->getActiveConfigPath());
    if (success) {
        ProxyLogger::getInstance().info("Xray started successfully");
        emit xrayStatusChanged(true);
    } else {
        ProxyLogger::getInstance().error("Failed to start Xray");
    }
    return success;
}

bool ProxyService::stopXray()
{
    ProxyLogger::getInstance().info("Stopping Xray");
    m_xrayController->stop();
    ProxyLogger::getInstance().info("Xray stopped");
    emit xrayStatusChanged(false);
    return true;
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

QMap<QString, QJsonObject> ProxyService::getAllConfigs() const
{
    ProxyLogger::getInstance().debug("Getting all configs");
    return m_configManager->getAllConfigs();
}

QMap<QString, QJsonObject> ProxyService::getConfigsByUuids(const QStringList &uuids) const
{
    ProxyLogger::getInstance().debug(QString("Getting configs for UUIDs: %1").arg(uuids.join(", ")));
    return m_configManager->getConfigsByUuids(uuids);
}

bool ProxyService::addConfigs(const QStringList &serializedConfigs)
{
    ProxyLogger::getInstance().info(QString("Adding %1 new config(s)").arg(serializedConfigs.size()));
    bool success = m_configManager->addConfigs(serializedConfigs);
    if (success) {
        ProxyLogger::getInstance().info("Configs added successfully");
        emit configsChanged();
    } else {
        ProxyLogger::getInstance().error("Failed to add configs");
    }
    return success;
}

bool ProxyService::removeConfig(const QString &uuid)
{
    ProxyLogger::getInstance().info(QString("Removing config with UUID: %1").arg(uuid));
    
    // Store current active config UUID before removal
    QString activeUuid = m_configManager->getActiveConfigUuid();

    // Try to remove the config
    bool removed = m_configManager->removeConfig(uuid);
    if (removed) {
        ProxyLogger::getInstance().info("Config removed successfully");
        emit configsChanged();
        if (uuid == activeUuid && isXrayRunning()) {
            ProxyLogger::getInstance().info("Removed active config, restarting Xray");
            stopXray();
            
            // Check if there are any configs left
            QString newActiveUuid = m_configManager->getActiveConfigUuid();
            if (!newActiveUuid.isEmpty()) {
                ProxyLogger::getInstance().info(QString("Starting Xray with new active config: %1").arg(newActiveUuid));
                return startXray();
            } else {
                ProxyLogger::getInstance().info("No configs left after removal");
            }
        }
    } else {
        ProxyLogger::getInstance().error(QString("Failed to remove config with UUID: %1").arg(uuid));
    }
    return removed;
}

bool ProxyService::activateConfig(const QString &uuid)
{
    ProxyLogger::getInstance().info(QString("Activating config with UUID: %1").arg(uuid));
    if (m_configManager->activateConfig(uuid)) {
        ProxyLogger::getInstance().info("Config activated successfully");
        emit configsChanged();
        // If config is successfully activated, restart Xray
        if (isXrayRunning()) {
            ProxyLogger::getInstance().info("Restarting Xray with new config");
            stopXray();
            return startXray();
        }
        return true;
    }
    ProxyLogger::getInstance().error(QString("Failed to activate config with UUID: %1").arg(uuid));
    return false;
}

QJsonObject ProxyService::getActiveConfig() const
{
    ProxyLogger::getInstance().debug("Getting active config");
    return m_configManager->getActiveConfig();
}

bool ProxyService::updateAllConfigs(const QStringList &serializedConfigs)
{
    ProxyLogger::getInstance().info(QString("Updating all configs with %1 new config(s)").arg(serializedConfigs.size()));
    bool success = m_configManager->updateAllConfigs(serializedConfigs);
    if (success) {
        ProxyLogger::getInstance().info("All configs updated successfully");
        emit configsChanged();
        if (isXrayRunning()) {
            ProxyLogger::getInstance().info("Restarting Xray with updated configs");
            stopXray();
            return startXray();
        }
    } else {
        ProxyLogger::getInstance().error("Failed to update all configs");
    }
    return success;
}

QString ProxyService::getActiveConfigUuid() const
{
    return m_configManager->getActiveConfigUuid();
}

int ProxyService::getConfigCount() const
{
    return m_configManager->getConfigCount();
} 