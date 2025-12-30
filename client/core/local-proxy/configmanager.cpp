#include "configmanager.h"
#include "core/serialization/serialization.h"
#include "proxylogger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QUuid>

ConfigManager::ConfigManager()
{
    ProxyLogger::getInstance().debug("Initializing ConfigManager");
    
    // Create configs directory if it doesn't exist
    QString configPath = getConfigsPath();
    ProxyLogger::getInstance().debug(QString("Ensuring config directory exists: %1").arg(configPath));
    QDir().mkpath(configPath);

    // Read active config UUID and initialize config count
    QJsonObject configsInfo = readConfigsInfo();
    m_activeConfigUuid = configsInfo["activeConfigUuid"].toString();
    m_configCount = configsInfo["configs"].toObject().size();
    ProxyLogger::getInstance().info(QString("Active config UUID: %1, Total configs: %2")
        .arg(m_activeConfigUuid.isEmpty() ? "none" : m_activeConfigUuid)
        .arg(m_configCount));
}

QString ConfigManager::getConfigsPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(configDir).filePath("xray_configs");
}

QString ConfigManager::getActiveConfigPath() const
{
    return QDir(getConfigsPath()).filePath("active_config.json");
}

QString ConfigManager::getConfigsInfoPath() const
{
    return QDir(getConfigsPath()).filePath("configs_info.json");
}

QJsonObject ConfigManager::readConfigsInfo() const
{
    QFile file(getConfigsInfoPath());

    if (!file.exists()) {
        ProxyLogger::getInstance().info("Configs info file doesn't exist, creating new one");
        // If file doesn't exist, return empty structure
        QJsonObject configsInfo;
        configsInfo["version"] = 1;
        configsInfo["configs"] = QJsonObject();
        return configsInfo;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        ProxyLogger::getInstance().error(QString("Failed to open configs info file: %1").arg(file.errorString()));
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        ProxyLogger::getInstance().error(QString("Failed to parse configs info file: %1").arg(parseError.errorString()));
        return QJsonObject();
    }

    ProxyLogger::getInstance().debug("Successfully read configs info file");
    return doc.object();
}

bool ConfigManager::writeConfigsInfo(const QJsonObject &configsInfo)
{
    QFile file(getConfigsInfoPath());

    if (!file.open(QIODevice::WriteOnly)) {
        ProxyLogger::getInstance().error(QString("Failed to open configs info file for writing: %1").arg(file.errorString()));
        return false;
    }

    m_configCount = configsInfo["configs"].toObject().size();
    ProxyLogger::getInstance().debug(QString("Updated config count: %1").arg(m_configCount));

    QJsonDocument doc(configsInfo);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    ProxyLogger::getInstance().debug("Successfully wrote configs info file");
    return true;
}

QJsonObject ConfigManager::readActiveConfig() const
{
    QFile file(getActiveConfigPath());

    if (!file.exists()) {
        ProxyLogger::getInstance().warning(QString("Active config file not found at: %1").arg(getActiveConfigPath()));
        return QJsonObject();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        ProxyLogger::getInstance().error(QString("Failed to open active config file: %1").arg(file.errorString()));
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        ProxyLogger::getInstance().error(QString("Failed to parse active config file: %1").arg(parseError.errorString()));
        return QJsonObject();
    }

    ProxyLogger::getInstance().debug("Successfully read active config file");
    return doc.object();
}

bool ConfigManager::writeActiveConfig(const QJsonObject &config)
{
    ProxyLogger::getInstance().info("Writing new active config");
    QFile file(getActiveConfigPath());

    if (!file.open(QIODevice::WriteOnly)) {
        ProxyLogger::getInstance().error(QString("Failed to open active config file for writing: %1").arg(file.errorString()));
        return false;
    }

    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    ProxyLogger::getInstance().debug("Successfully wrote active config file");
    return true;
}

bool ConfigManager::removeActiveConfigFile()
{
    ProxyLogger::getInstance().info("Removing active config file");
    QFile file(getActiveConfigPath());
    
    if (file.exists()) {
        if (file.remove()) {
            ProxyLogger::getInstance().debug("Successfully removed active config file");
            return true;
        } else {
            ProxyLogger::getInstance().error(QString("Failed to remove active config file: %1").arg(file.errorString()));
            return false;
        }
    }
    
    ProxyLogger::getInstance().debug("Active config file does not exist, nothing to remove");
    return true;
}

QString ConfigManager::generateUuid() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString ConfigManager::getProtocolFromSerializedConfig(const QString &config) const
{
    if (config.startsWith("vless://")) return "vless";
    if (config.startsWith("vmess://")) return "vmess";
    if (config.startsWith("trojan://")) return "trojan";
    if (config.startsWith("ss://")) return "ss";
    return QString();
}

QJsonObject ConfigManager::deserializeConfig(const QString &configStr, QString *prefix, QString *errorMsg)
{
    ProxyLogger::getInstance().debug("Deserializing config");
    QJsonObject outConfig;
    
    QString localPrefix;
    QString localErrorMsg;
    
    QString* safePrefix = prefix ? prefix : &localPrefix;
    QString* safeErrorMsg = errorMsg ? errorMsg : &localErrorMsg;

    if (configStr.startsWith("vless://")) {
        ProxyLogger::getInstance().debug("Deserializing VLESS config");
        outConfig = amnezia::serialization::vless::Deserialize(configStr, safePrefix, safeErrorMsg);
        if (safePrefix->contains(QRegularExpression("%[0-9A-Fa-f]{2}"))) {
            *safePrefix = QString::fromUtf8(QByteArray::fromPercentEncoding(safePrefix->toUtf8()));
        }
    }

    if (configStr.startsWith("vmess://") && configStr.contains("@")) {
        ProxyLogger::getInstance().debug("Deserializing new VMess config");
        outConfig = amnezia::serialization::vmess_new::Deserialize(configStr, safePrefix, safeErrorMsg);
    }

    else if (configStr.startsWith("vmess://")) {
        ProxyLogger::getInstance().debug("Deserializing VMess config");
        outConfig = amnezia::serialization::vmess::Deserialize(configStr, safePrefix, safeErrorMsg);
    }

    if (configStr.startsWith("trojan://")) {
        ProxyLogger::getInstance().debug("Deserializing Trojan config");
        outConfig = amnezia::serialization::trojan::Deserialize(configStr, safePrefix, safeErrorMsg);
    }

    if (configStr.startsWith("ss://") && !configStr.contains("plugin=")) {
        ProxyLogger::getInstance().debug("Deserializing Shadowsocks config");
        outConfig = amnezia::serialization::ss::Deserialize(configStr, safePrefix, safeErrorMsg);
        if (safePrefix->contains(QRegularExpression("%[0-9A-Fa-f]{2}"))) {
            *safePrefix = QString::fromUtf8(QByteArray::fromPercentEncoding(safePrefix->toUtf8()));
        }
    }

    if (!safeErrorMsg->isEmpty()) {
        ProxyLogger::getInstance().error(QString("Config deserialization error: %1").arg(*safeErrorMsg));
    }

    return outConfig;
}

QJsonObject ConfigManager::addInbounds(const QJsonObject &config)
{
    ProxyLogger::getInstance().debug("Adding inbounds configuration");
    QJsonObject resultConfig = config;

    QJsonArray inbounds;
    QJsonObject socksInbound;
    socksInbound["listen"] = "127.0.0.1";
    socksInbound["port"] = 10808;
    socksInbound["protocol"] = "socks";

    QJsonObject settings;
    settings["udp"] = true;
    socksInbound["settings"] = settings;

    inbounds.append(socksInbound);
    resultConfig["inbounds"] = inbounds;

    ProxyLogger::getInstance().debug("Successfully added SOCKS inbound configuration (port: 10808)");
    return resultConfig;
}

bool ConfigManager::addConfigs(const QStringList &serializedConfigs)
{
    ProxyLogger::getInstance().info(QString("Adding %1 new config(s)").arg(serializedConfigs.size()));
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();
    QString activeUuid = getActiveConfigUuid();
    QString firstUuid;

    QSet<QString> existingConfigs;
    for (auto it = configs.begin(); it != configs.end(); ++it) {
        QJsonObject configInfo = it.value().toObject();
        existingConfigs.insert(configInfo["serializedConfig"].toString());
    }

    for (const QString &serializedConfig : serializedConfigs)
    {
        if (existingConfigs.contains(serializedConfig)) {
            ProxyLogger::getInstance().info("Skipping duplicate config");
            continue;
        }

        QString uuid = generateUuid();
        ProxyLogger::getInstance().debug(QString("Generated new UUID: %1").arg(uuid));
        
        if (firstUuid.isEmpty()) {
            firstUuid = uuid;
        }

        QString prefix;
        QString errorMsg;
        ProxyLogger::getInstance().debug(QString("Deserializing config: %1").arg(serializedConfig.left(50) + "..."));
        QJsonObject config = deserializeConfig(serializedConfig, &prefix, &errorMsg);

        if (!errorMsg.isEmpty()) {
            ProxyLogger::getInstance().error(QString("Failed to deserialize config: %1").arg(errorMsg));
            continue;
        }

        QJsonObject currentConfigInfo;
        currentConfigInfo["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        currentConfigInfo["lastUsed"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        currentConfigInfo["protocol"] = getProtocolFromSerializedConfig(serializedConfig);
        currentConfigInfo["serializedConfig"] = serializedConfig;
        currentConfigInfo["name"] = prefix.isEmpty() ? uuid : prefix;
        currentConfigInfo["isActive"] = false;

        ProxyLogger::getInstance().info(QString("Adding config: UUID=%1, Protocol=%2, Name=%3")
            .arg(uuid)
            .arg(currentConfigInfo["protocol"].toString())
            .arg(currentConfigInfo["name"].toString()));

        configs[uuid] = currentConfigInfo;
    }

    configsInfo["configs"] = configs;
    ProxyLogger::getInstance().debug("Writing updated configs info to file");
    if (!writeConfigsInfo(configsInfo)) {
        ProxyLogger::getInstance().error("Failed to write configs info");
        return false;
    }

    // If there's no active config, activate the first added one    
    if (activeUuid.isEmpty() && !firstUuid.isEmpty())
    {
        ProxyLogger::getInstance().info(QString("No active config, activating first added config: %1").arg(firstUuid));
        return activateConfig(firstUuid);
    }

    return true;
}

bool ConfigManager::removeConfig(const QString &uuid)
{
    ProxyLogger::getInstance().info(QString("Removing config with UUID: %1").arg(uuid));
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();

    // Check if config exists
    if (!configs.contains(uuid))
    {
        ProxyLogger::getInstance().warning(QString("Config with UUID %1 not found").arg(uuid));
        return false;
    }

    QJsonObject configToRemove = configs[uuid].toObject();
    ProxyLogger::getInstance().info(QString("Removing config: Name=%1, Protocol=%2")
        .arg(configToRemove["name"].toString())
        .arg(configToRemove["protocol"].toString()));

    // Store current active config UUID
    bool needToActivateNew = (getActiveConfigUuid() == uuid);
    
    // Remove config from the list
    configs.remove(uuid);
    
    // Save updated configs list (without changing activeConfigUuid)
    configsInfo["configs"] = configs;
    ProxyLogger::getInstance().debug("Writing updated configs info to file");
    if (!writeConfigsInfo(configsInfo))
    {
        ProxyLogger::getInstance().error("Failed to write configs info");
        return false;
    }
    
    // If active config was removed, activate a new one
    if (needToActivateNew)
    {
        ProxyLogger::getInstance().info("Removed active config, need to activate a new one");
        if (configs.isEmpty())
        {
            ProxyLogger::getInstance().info("No configs left, clearing active config");
            if (!activateConfig(QString()))
            {
                ProxyLogger::getInstance().error("Failed to clear active config");
                return false;
            }
        }
        else
        {
            QString newActiveUuid = configs.keys().first();
            ProxyLogger::getInstance().info(QString("Activating new config: %1").arg(newActiveUuid));
            if (!activateConfig(newActiveUuid))
            {
                ProxyLogger::getInstance().error(QString("Failed to activate new config: %1").arg(newActiveUuid));
                return false;
            }
        }
    }
    
    return true;
}

bool ConfigManager::activateConfig(const QString &uuid)
{
    ProxyLogger::getInstance().info(QString("Activating config: %1").arg(uuid.isEmpty() ? "none" : uuid));
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();

    // Reset isActive flag for all configs
    for (auto it = configs.begin(); it != configs.end(); ++it) {
        QJsonObject config = it.value().toObject();
        config["isActive"] = false;
        it.value() = config;
    }

    // If uuid is empty, just reset active config
    if (uuid.isEmpty())
    {
        ProxyLogger::getInstance().info("Resetting active config");
        m_activeConfigUuid = QString();
        configsInfo["activeConfigUuid"] = QString();
        configsInfo["configs"] = configs;
        
        // Write changes to the configs info file
        if (!writeConfigsInfo(configsInfo)) {
            ProxyLogger::getInstance().error("Failed to write configs info file");
            return false;
        }
        
        // Remove the active config file
        ProxyLogger::getInstance().debug("Removing active config file");
        return removeActiveConfigFile();
    }

    // Check if config exists
    if (!configs.contains(uuid))
    {
        ProxyLogger::getInstance().error(QString("Config with UUID %1 not found").arg(uuid));
        return false;
    }

    QJsonObject currentConfigInfo = configs[uuid].toObject();
    ProxyLogger::getInstance().info(QString("Activating config: Name=%1, Protocol=%2")
        .arg(currentConfigInfo["name"].toString())
        .arg(currentConfigInfo["protocol"].toString()));

    QString serializedConfig = currentConfigInfo["serializedConfig"].toString();

    // Deserialize config and add inbounds
    QString prefix;
    QString errorMsg;
    ProxyLogger::getInstance().debug("Deserializing config for activation");
    QJsonObject currentConfig = deserializeConfig(serializedConfig, &prefix, &errorMsg);
    if (currentConfig.isEmpty())
    {
        ProxyLogger::getInstance().error(QString("Failed to deserialize config: %1").arg(errorMsg));
        return false;
    }

    ProxyLogger::getInstance().debug("Adding inbounds to config");
    currentConfig = addInbounds(currentConfig);

    // Update lastUsed and isActive
    currentConfigInfo["lastUsed"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    currentConfigInfo["isActive"] = true;
    configs[uuid] = currentConfigInfo;
    configsInfo["configs"] = configs;

    // Update active config
    m_activeConfigUuid = uuid;
    configsInfo["activeConfigUuid"] = uuid;

    // Save changes
    ProxyLogger::getInstance().debug("Writing updated configs info");
    if (!writeConfigsInfo(configsInfo))
    {
        ProxyLogger::getInstance().error("Failed to write configs info file");
        return false;
    }

    ProxyLogger::getInstance().debug("Writing new active config file");
    return writeActiveConfig(currentConfig);
}

QJsonObject ConfigManager::getActiveConfig() const
{
    ProxyLogger::getInstance().debug("Getting active config info");
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();
    QString activeUuid = getActiveConfigUuid();

    if (activeUuid.isEmpty() || !configs.contains(activeUuid)) {
        ProxyLogger::getInstance().debug("No active config found");
        return QJsonObject();
    }

    ProxyLogger::getInstance().debug(QString("Retrieved active config info for UUID: %1").arg(activeUuid));
    QJsonObject result = configs[activeUuid].toObject();
    result["id"] = activeUuid;
    return result;
}

QMap<QString, QJsonObject> ConfigManager::getAllConfigs() const
{
    ProxyLogger::getInstance().debug("Getting all configs");
    QJsonObject configsInfo = readConfigsInfo();
    QJsonObject configs = configsInfo["configs"].toObject();

    QMap<QString, QJsonObject> result;
    for (auto it = configs.begin(); it != configs.end(); ++it)
    {
        result[it.key()] = it.value().toObject();
    }

    ProxyLogger::getInstance().debug(QString("Retrieved %1 configs").arg(result.size()));
    return result;
}

QMap<QString, QJsonObject> ConfigManager::getConfigsByUuids(const QStringList &uuids) const
{
    ProxyLogger::getInstance().debug(QString("Getting configs for %1 UUIDs").arg(uuids.size()));
    QMap<QString, QJsonObject> allConfigs = getAllConfigs();
    if (uuids.isEmpty())
    {
        ProxyLogger::getInstance().debug("UUID list is empty, returning all configs");
        return allConfigs;
    }

    QMap<QString, QJsonObject> result;
    for (const QString &uuid : uuids)
    {
        if (allConfigs.contains(uuid))
        {
            ProxyLogger::getInstance().debug(QString("Found config for UUID: %1").arg(uuid));
            result[uuid] = allConfigs[uuid];
        }
        else
        {
            ProxyLogger::getInstance().warning(QString("Config not found for UUID: %1").arg(uuid));
            result[uuid] = QJsonObject();
        }
    }

    ProxyLogger::getInstance().debug(QString("Retrieved %1 configs out of %2 requested").arg(result.size()).arg(uuids.size()));
    return result;
}

bool ConfigManager::updateAllConfigs(const QStringList &serializedConfigs)
{
    ProxyLogger::getInstance().info(QString("Updating all configs with %1 new config(s)").arg(serializedConfigs.size()));
    
    ProxyLogger::getInstance().debug("Clearing existing configs");
    if (!clearConfigs()) {
        ProxyLogger::getInstance().error("Failed to clear existing configs");
        return false;
    }
    
    ProxyLogger::getInstance().debug("Adding new configs");
    bool success = addConfigs(serializedConfigs);
    if (success) {
        ProxyLogger::getInstance().info("Successfully updated all configs");
    } else {
        ProxyLogger::getInstance().error("Failed to add new configs");
    }
    return success;
}

bool ConfigManager::clearConfigs()
{
    ProxyLogger::getInstance().info("Clearing all configs");
    QJsonObject configsInfo = readConfigsInfo();
    configsInfo["configs"] = QJsonObject();
    
    if (!writeConfigsInfo(configsInfo)) {
        ProxyLogger::getInstance().error("Failed to clear configs info");
        return false;
    }
    
    ProxyLogger::getInstance().debug("Resetting active config");
    bool success = activateConfig(QString());
    if (success) {
        ProxyLogger::getInstance().info("Successfully cleared all configs");
    } else {
        ProxyLogger::getInstance().error("Failed to reset active config");
    }
    return success;
}