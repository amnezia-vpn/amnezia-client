#ifndef PROXYCONFIGMANAGER_H
#define PROXYCONFIGMANAGER_H

#include <optional>

#include <QJsonObject>
#include <QString>

class SecureServersRepository;
class SecureAppSettingsRepository;

class ProxyConfigManager
{
public:
    struct ConfigData
    {
        QString serializedConfig;
        QJsonObject parsedConfig;
        int proxyPort = 0;
    };

    ProxyConfigManager(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository);

    std::optional<ConfigData> buildConfig(QString &errorDescription) const;

private:
    std::optional<QString> extractSerializedXrayConfig(const QJsonObject &server) const;
    std::optional<QString> fetchSerializedXrayConfigFromGateway(const QJsonObject &server, QString &errorDescription) const;
    std::optional<int> selectProxyPort(QString &errorDescription) const;
    bool applyProxyPortToConfig(QJsonObject &config, int port) const;

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // PROXYCONFIGMANAGER_H
