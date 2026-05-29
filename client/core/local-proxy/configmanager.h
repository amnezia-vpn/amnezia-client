#pragma once

#include <optional>

#include <QJsonObject>
#include <QString>

class SecureServersRepository;
class SecureAppSettingsRepository;

class ConfigManager {
public:
    struct ConfigData {
        QString ownerId;
        QString serverName;
        QString serializedConfig;
        QJsonObject parsedConfig;
    };

    ConfigManager(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository);

    std::optional<ConfigData> buildConfig(QString &errorDescription) const;
    std::optional<ConfigData> buildConfigWithFetch(QString &errorDescription) const;
    bool writeTempConfig(const QString &serializedConfig, QString &configPath, QString &errorDescription) const;
    bool removeTempConfig() const;
    QString tempConfigPath() const;

private:
    std::optional<QString> extractSerializedXrayConfig(const QJsonObject &server) const;
    std::optional<QString> fetchSerializedXrayConfigFromGateway(const QJsonObject &server, QString &errorDescription) const;
    QString tempDirectory() const;
    bool applyProxyPortToConfig(QJsonObject &config, int port) const;
    QString serializeConfig(const QJsonObject &config) const;

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
};
