#pragma once

#include <QJsonObject>
#include <QString>
#include <QFile>
#include <QMap>

class ConfigManager {
public:
    ConfigManager();

    // Main config operations
    bool addConfigs(const QStringList &serializedConfigs);
    bool removeConfig(const QString &uuid);
    bool activateConfig(const QString &uuid);
    bool updateAllConfigs(const QStringList &serializedConfigs);
    bool clearConfigs();

    // Information retrieval
    QString getActiveConfigUuid() const { return m_activeConfigUuid; }
    QJsonObject getActiveConfig() const;
    QMap<QString, QJsonObject> getAllConfigs() const;
    QMap<QString, QJsonObject> getConfigsByUuids(const QStringList &uuids) const;
    QString getActiveConfigPath() const;
    int getConfigCount() const { return m_configCount; }

private:
    // File paths
    QString getConfigsPath() const;
    QString getConfigsInfoPath() const;

    // File operations
    QJsonObject readConfigsInfo() const;
    bool writeConfigsInfo(const QJsonObject &configsInfo);
    QJsonObject readActiveConfig() const;
    bool writeActiveConfig(const QJsonObject &config);
    bool removeActiveConfigFile();

    // Helper methods
    QString generateUuid() const;
    QString getProtocolFromSerializedConfig(const QString &config) const;
    QJsonObject deserializeConfig(const QString &configStr, QString *prefix = nullptr, QString *errorMsg = nullptr);
    QJsonObject addInbounds(const QJsonObject &config);

    QString m_activeConfigUuid;
    int m_configCount{0};
}; 