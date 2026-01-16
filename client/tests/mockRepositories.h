#ifndef MOCK_REPOSITORIES_H
#define MOCK_REPOSITORIES_H

#include <QSettings>
#include <QTemporaryFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "core/repositories/serversRepository.h"
#include "core/repositories/appSettingsRepository.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "containers/containers_defs.h"

using namespace amnezia;

/**
 * Mock repository для тестирования серверов
 * Использует временное хранилище в памяти
 */
class MockServersRepository : public ServersRepository
{
private:
    QVector<ServerConfig> m_servers;
    int m_defaultServerIndex = -1;

public:
    MockServersRepository() = default;
    ~MockServersRepository() override = default;

    void addServer(const ServerConfig &server) override {
        m_servers.append(server);
        if (m_defaultServerIndex == -1) {
            m_defaultServerIndex = 0;
        }
    }

    void editServer(int index, const ServerConfig &server) override {
        if (index >= 0 && index < m_servers.size()) {
            m_servers[index] = server;
        }
    }

    void removeServer(int index) override {
        if (index >= 0 && index < m_servers.size()) {
            m_servers.removeAt(index);
            if (m_defaultServerIndex >= m_servers.size()) {
                m_defaultServerIndex = m_servers.size() - 1;
            }
        }
    }

    ServerConfig server(int index) const override {
        if (index >= 0 && index < m_servers.size()) {
            return m_servers[index];
        }
        return SelfHostedServerConfig{};
    }

    QVector<ServerConfig> servers() const override {
        return m_servers;
    }

    int serversCount() const override {
        return m_servers.size();
    }

    int defaultServerIndex() const override {
        return m_defaultServerIndex;
    }

    void setDefaultServer(int index) override {
        if (index >= -1 && index < m_servers.size()) {
            m_defaultServerIndex = index;
        }
    }

    void setDefaultContainer(int serverIndex, DockerContainer container) override {
        if (serverIndex >= 0 && serverIndex < m_servers.size()) {
            ServerConfigUtils::visit(m_servers[serverIndex], [container](auto& config) {
                config.defaultContainer = container;
            });
        }
    }

    ContainerConfig containerConfig(int serverIndex, DockerContainer container) const override {
        if (serverIndex >= 0 && serverIndex < m_servers.size()) {
            return ServerConfigUtils::visit(m_servers[serverIndex], [container](const auto& config) {
                auto it = config.containers.find(container);
                if (it != config.containers.end()) {
                    return it.value();
                }
                return ContainerConfig{};
            });
        }
        return ContainerConfig{};
    }

    void setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config) override {
        if (serverIndex >= 0 && serverIndex < m_servers.size()) {
            ServerConfigUtils::visit(m_servers[serverIndex], [container, &config](auto& serverConfig) {
                serverConfig.containers[container] = config;
            });
        }
    }

    void clearLastConnectionConfig(int serverIndex, DockerContainer container) override {
        // Not implemented for tests
    }

    ServerCredentials serverCredentials(int index) const override {
        if (index >= 0 && index < m_servers.size()) {
            return ServerConfigUtils::visit(m_servers[index], [](const auto& config) -> ServerCredentials {
                using ConfigType = std::decay_t<decltype(config)>;
                
                if constexpr (std::is_same_v<ConfigType, SelfHostedServerConfig>) {
                    return ServerCredentials {
                        config.hostName,
                        config.userName.value_or(""),
                        config.password.value_or(""),
                        config.port.value_or(22)
                    };
                } else {
                    // For API and Native configs
                    return ServerCredentials {
                        config.hostName,
                        "",  // no username
                        "",  // no password
                        22   // default port
                    };
                }
            });
        }
        return ServerCredentials{};
    }

    bool hasServerWithVpnKey(const QString &vpnKey) const override {
        // Not implemented for tests
        return false;
    }

    // Utility для очистки данных между тестами
    void clear() {
        m_servers.clear();
        m_defaultServerIndex = -1;
    }
};

/**
 * Mock repository для настроек приложения
 * Использует значения по умолчанию
 */
class MockAppSettingsRepository : public AppSettingsRepository
{
public:
    MockAppSettingsRepository() = default;
    ~MockAppSettingsRepository() override = default;

    QLocale getAppLanguage() const override { return QLocale(QLocale::English); }
    void setAppLanguage(QLocale locale) override {}

    bool useAmneziaDns() const override { return true; }
    void setUseAmneziaDns(bool enabled) override {}
    QStringList getAllowedDnsServers() const override { return {}; }
    void setAllowedDnsServers(const QStringList &servers) override {}
    QString primaryDns() const override { return "1.1.1.1"; }
    void setPrimaryDns(const QString &dns) override {}
    QString secondaryDns() const override { return "1.0.0.1"; }
    void setSecondaryDns(const QString &dns) override {}

    RouteMode routeMode() const override { return RouteMode::VpnAllSites; }
    void setRouteMode(RouteMode mode) override {}
    bool addVpnSite(RouteMode mode, const QString &site, const QString &ip = "") override { return true; }
    void addVpnSites(RouteMode mode, const QMap<QString, QString> &sites) override {}
    void removeVpnSite(RouteMode mode, const QString &site) override {}
    void removeAllVpnSites(RouteMode mode) override {}
    QVariantMap vpnSites(RouteMode mode) const override { return {}; }
    bool isSitesSplitTunnelingEnabled() const override { return false; }
    void setSitesSplitTunnelingEnabled(bool enabled) override {}

    AppsRouteMode appsRouteMode() const override { return AppsRouteMode::VpnAllApps; }
    void setAppsRouteMode(AppsRouteMode mode) override {}
    void setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps) override {}
    QVector<InstalledAppInfo> vpnApps(AppsRouteMode mode) const override { return {}; }
    bool isAppsSplitTunnelingEnabled() const override { return false; }
    void setAppsSplitTunnelingEnabled(bool enabled) override {}

    QString getGatewayEndpoint(bool isTestPurchase = false) const override { return ""; }
    void setGatewayEndpoint(const QString &endpoint) override {}
    void resetGatewayEndpoint() override {}
    void setDevGatewayEndpoint() override {}
    bool isDevGatewayEnv(bool isTestPurchase = false) const override { return false; }
    void toggleDevGatewayEnv(bool enabled) override {}

    bool isKillSwitchEnabled() const override { return false; }
    void setKillSwitchEnabled(bool enabled) override {}
    bool isStrictKillSwitchEnabled() const override { return false; }
    void setStrictKillSwitchEnabled(bool enabled) override {}

    bool isAutoConnect() const override { return false; }
    void setAutoConnect(bool enabled) override {}
    bool isStartMinimized() const override { return false; }
    void setStartMinimized(bool enabled) override {}
    bool isScreenshotsEnabled() const override { return true; }
    void setScreenshotsEnabled(bool enabled) override {}
    bool isSaveLogs() const override { return false; }
    void setSaveLogs(bool enabled) override {}
    QDateTime getLogEnableDate() const override { return QDateTime(); }
    void setLogEnableDate(const QDateTime &date) override {}

    QString getInstallationUuid(bool createIfNotExists) const override { return "test-uuid"; }
    bool isHomeAdLabelVisible() const override { return false; }
    void disableHomeAdLabel() override {}
    bool isPremV1MigrationReminderActive() const override { return false; }
    void disablePremV1MigrationReminder() override {}
    QByteArray backupAppConfig() const override { return QByteArray(); }
    bool restoreAppConfig(const QByteArray &cfg) override { return true; }
    void clearSettings() override {}

    QString nextAvailableServerName() const override {
        static int counter = 0;
        return QString("Test Server %1").arg(++counter);
    }
};

#endif // MOCK_REPOSITORIES_H

