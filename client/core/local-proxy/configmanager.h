#pragma once

#include <memory>
#include <optional>

#include <QJsonObject>
#include <QString>

class Settings;

class ConfigManager {
public:
    struct ConfigData {
        QString ownerUuid;
        QString serverName;
        QString serializedConfig;
        QJsonObject parsedConfig;
    };

    explicit ConfigManager(const std::shared_ptr<Settings> &settings);

    std::optional<ConfigData> buildConfig(QString &errorDescription) const;
    std::optional<ConfigData> buildConfigWithFetch(QString &errorDescription) const;
    bool writeTempConfig(const QString &serializedConfig, QString &configPath, QString &errorDescription) const;
    bool removeTempConfig() const;
    QString tempConfigPath() const;

private:
    std::optional<QJsonObject> findServerByUuid(const QString &uuid) const;
    std::optional<QString> extractSerializedXrayConfig(const QJsonObject &server) const;
    std::optional<QString> fetchSerializedXrayConfigFromGateway(const QJsonObject &server, QString &errorDescription) const;
    QString tempDirectory() const;

    std::shared_ptr<Settings> m_settings;
};