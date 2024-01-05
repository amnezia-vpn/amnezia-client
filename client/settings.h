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

    ServerCredentials defaultServerCredentials();
    ServerCredentials serverCredentials(int index);

    QJsonArray serversArray()
    {
        return QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
    }
    void setServersArray(const QJsonArray &servers)
    {
        setValue("Servers/serversList", QJsonDocument(servers).toJson());
    }

    // Servers section
    int serversCount();
    QJsonObject server(int index);
    void addServer(const QJsonObject &server);
    void removeServer(int index);
    bool editServer(int index, const QJsonObject &server);

    int defaultServerIndex()
    {
        return value("Servers/defaultServerIndex", 0).toInt();
    }
    void setDefaultServer(int index)
    {
        setValue("Servers/defaultServerIndex", index);
    }
    QJsonObject defaultServer()
    {
        return server(defaultServerIndex());
    }

    void setDefaultContainer(int serverIndex, DockerContainer container);
    DockerContainer defaultContainer(int serverIndex);
    QString defaultContainerName(int serverIndex);

    QMap<DockerContainer, QJsonObject> containers(int serverIndex);
    void setContainers(int serverIndex, const QMap<DockerContainer, QJsonObject> &containers);

    QJsonObject containerConfig(int serverIndex, DockerContainer container);
    void setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config);
    void removeContainerConfig(int serverIndex, DockerContainer container);

    QJsonObject protocolConfig(int serverIndex, DockerContainer container, Proto proto);
    void setProtocolConfig(int serverIndex, DockerContainer container, Proto proto, const QJsonObject &config);

    void clearLastConnectionConfig(int serverIndex, DockerContainer container, Proto proto = Proto::Any);

    bool haveAuthData(int serverIndex);
    QString nextAvailableServerName();

    // App settings section
    bool isAutoConnect()
    {
        return value("Conf/autoConnect", false).toBool();
    }
    void setAutoConnect(bool enabled)
    {
        setValue("Conf/autoConnect", enabled);
    }

    bool isStartMinimized()
    {
        return value("Conf/startMinimized", false).toBool();
    }
    void setStartMinimized(bool enabled)
    {
        setValue("Conf/startMinimized", enabled);
    }

    bool isSaveLogs()
    {
        return value("Conf/saveLogs", false).toBool();
    }
    void setSaveLogs(bool enabled);

    enum RouteMode {
        VpnAllSites,
        VpnOnlyForwardSites,
        VpnAllExceptSites
    };
    Q_ENUM(RouteMode)

    QString routeModeString(RouteMode mode);

    RouteMode routeMode();
    void setRouteMode(RouteMode mode) { setValue("Conf/routeMode", mode); }

    QVariantMap vpnSites(RouteMode mode)
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
    QStringList getVpnIps(RouteMode mode);
    void removeVpnSite(RouteMode mode, const QString &site);

    void addVpnIps(RouteMode mode, const QStringList &ip);
    void removeVpnSites(RouteMode mode, const QStringList &sites);
    void removeAllVpnSites(RouteMode mode);

    bool useAmneziaDns()
    {
        return value("Conf/useAmneziaDns", true).toBool();
    }
    void setUseAmneziaDns(bool enabled)
    {
        setValue("Conf/useAmneziaDns", enabled);
    }

    QString primaryDns();
    QString secondaryDns();

    // QString primaryDns() { return m_primaryDns; }
    void setPrimaryDns(const QString &primaryDns)
    {
        setValue("Conf/primaryDns", primaryDns);
    }

    // QString secondaryDns() { return m_secondaryDns; }
    void setSecondaryDns(const QString &secondaryDns)
    {
        setValue("Conf/secondaryDns", secondaryDns);
    }

    static const char cloudFlareNs1[];
    static const char cloudFlareNs2[];

    //    static constexpr char openNicNs5[] = "94.103.153.176";
    //    static constexpr char openNicNs13[] = "144.76.103.143";

    QByteArray backupAppConfig()
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

    bool isScreenshotsEnabled()
    {
        return value("Conf/screenshotsEnabled", false).toBool();
    }
    void setScreenshotsEnabled(bool enabled)
    {
        setValue("Conf/screenshotsEnabled", enabled);
    }

    void clearSettings();

signals:
    void saveLogsChanged();

private:
    SecureQSettings m_settings;

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant());
    void setValue(const QString &key, const QVariant &value);
};

#endif // SETTINGS_H
