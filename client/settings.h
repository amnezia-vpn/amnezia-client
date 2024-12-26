#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QString>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "secure_qsettings.h"

using namespace amnezia;

class QSettings;

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject *parent = nullptr);

    ServerCredentials defaultServerCredentials() const;
    ServerCredentials serverCredentials(int index) const;

    QJsonArray serversArray() const
    {
        return QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
    }
    void setServersArray(const QJsonArray &servers)
    {
        setValue("Servers/serversList", QJsonDocument(servers).toJson());
    }

    // Servers section
    int serversCount() const;
    QJsonObject server(int index) const;
    void addServer(const QJsonObject &server);
    void removeServer(int index);
    bool editServer(int index, const QJsonObject &server);

    int defaultServerIndex() const
    {
        return value("Servers/defaultServerIndex", 0).toInt();
    }
    void setDefaultServer(int index)
    {
        setValue("Servers/defaultServerIndex", index);
    }
    QJsonObject defaultServer() const
    {
        return server(defaultServerIndex());
    }

    void setDefaultContainer(int serverIndex, DockerContainer container);
    DockerContainer defaultContainer(int serverIndex) const;
    QString defaultContainerName(int serverIndex) const;

    QMap<DockerContainer, QJsonObject> containers(int serverIndex) const;
    void setContainers(int serverIndex, const QMap<DockerContainer, QJsonObject> &containers);

    QJsonObject containerConfig(int serverIndex, DockerContainer container);
    void setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config);
    void removeContainerConfig(int serverIndex, DockerContainer container);

    QJsonObject protocolConfig(int serverIndex, DockerContainer container, Proto proto);
    void setProtocolConfig(int serverIndex, DockerContainer container, Proto proto, const QJsonObject &config);

    void clearLastConnectionConfig(int serverIndex, DockerContainer container, Proto proto = Proto::Any);

    bool haveAuthData(int serverIndex) const;
    QString nextAvailableServerName() const;

    // App settings section
    bool isAutoConnect() const
    {
        return value("Conf/autoConnect", false).toBool();
    }
    void setAutoConnect(bool enabled)
    {
        setValue("Conf/autoConnect", enabled);
    }

    bool isStartMinimized() const
    {
        return value("Conf/startMinimized", false).toBool();
    }
    void setStartMinimized(bool enabled)
    {
        setValue("Conf/startMinimized", enabled);
    }

    bool isSaveLogs() const
    {
        return value("Conf/saveLogs", false).toBool();
    }
    void setSaveLogs(bool enabled);

    QDateTime getLogEnableDate();
    void setLogEnableDate(QDateTime date);

    enum RouteMode {
        VpnAllSites,
        VpnOnlyForwardSites,
        VpnAllExceptSites
    };
    Q_ENUM(RouteMode)

    QString routeModeString(RouteMode mode) const;

    RouteMode routeMode() const;
    void setRouteMode(RouteMode mode) { setValue("Conf/routeMode", mode); }

    bool isSitesSplitTunnelingEnabled() const;
    void setSitesSplitTunnelingEnabled(bool enabled);

    QVariantMap vpnSites(RouteMode mode) const
    {
        return value("Conf/" + routeModeString(mode)).toMap();
    }
    void setVpnSites(RouteMode mode, const QVariantMap &sites)
    {
        setValue("Conf/" + routeModeString(mode), sites);
        m_settings.sync();
    }
    bool addVpnSite(RouteMode mode, const QString &site, const QString &ip = "");
    void addVpnSites(RouteMode mode, const QMap<QString, QString> &sites); // map <site, ip>
    QStringList getVpnIps(RouteMode mode) const;
    void removeVpnSite(RouteMode mode, const QString &site);

    void addVpnIps(RouteMode mode, const QStringList &ip);
    void removeVpnSites(RouteMode mode, const QStringList &sites);
    void removeAllVpnSites(RouteMode mode);

    bool useAmneziaDns() const
    {
        return value("Conf/useAmneziaDns", true).toBool();
    }
    void setUseAmneziaDns(bool enabled)
    {
        setValue("Conf/useAmneziaDns", enabled);
    }

    QString primaryDns() const;
    QString secondaryDns() const;

    // QString primaryDns() const { return m_primaryDns; }
    void setPrimaryDns(const QString &primaryDns)
    {
        setValue("Conf/primaryDns", primaryDns);
    }

    // QString secondaryDns() const { return m_secondaryDns; }
    void setSecondaryDns(const QString &secondaryDns)
    {
        setValue("Conf/secondaryDns", secondaryDns);
    }

    //    static constexpr char openNicNs5[] = "94.103.153.176";
    //    static constexpr char openNicNs13[] = "144.76.103.143";

    QByteArray backupAppConfig() const
    {
        return m_settings.backupAppConfig();
    }
    bool restoreAppConfig(const QByteArray &cfg)
    {
        return m_settings.restoreAppConfig(cfg);
    }

    QLocale getAppLanguage()
    {
        return value("Conf/appLanguage", QLocale()).toLocale();
    };
    void setAppLanguage(QLocale locale)
    {
        setValue("Conf/appLanguage", locale);
    };

    bool isScreenshotsEnabled() const
    {
        return value("Conf/screenshotsEnabled", true).toBool();
    }
    void setScreenshotsEnabled(bool enabled)
    {
        setValue("Conf/screenshotsEnabled", enabled);
        emit screenshotsEnabledChanged(enabled);
    }

    void clearSettings();

    enum AppsRouteMode {
        VpnAllApps,
        VpnOnlyForwardApps,
        VpnAllExceptApps
    };
    Q_ENUM(AppsRouteMode)

    QString appsRouteModeString(AppsRouteMode mode) const;

    AppsRouteMode getAppsRouteMode() const;
    void setAppsRouteMode(AppsRouteMode mode);

    QVector<InstalledAppInfo> getVpnApps(AppsRouteMode mode) const;
    void setVpnApps(AppsRouteMode mode, const QVector<InstalledAppInfo> &apps);

    bool isAppsSplitTunnelingEnabled() const;
    void setAppsSplitTunnelingEnabled(bool enabled);

    bool isKillSwitchEnabled() const;
    void setKillSwitchEnabled(bool enabled);
    QString getInstallationUuid(const bool needCreate);

    void resetGatewayEndpoint();
    void setGatewayEndpoint(const QString &endpoint);
    void setDevGatewayEndpoint();
    QString getGatewayEndpoint();
    bool isDevGatewayEnv();
    void toggleDevGatewayEnv(bool enabled);

    bool isHomeAdLabelVisible();
    void disableHomeAdLabel();

signals:
    void saveLogsChanged(bool enabled);
    void screenshotsEnabledChanged(bool enabled);
    void serverRemoved(int serverIndex);
    void settingsCleared();

private:
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

    void setInstallationUuid(const QString &uuid);

    mutable SecureQSettings m_settings;

    QString m_gatewayEndpoint;
    bool m_isDevGatewayEnv = false;
};

#endif // SETTINGS_H
