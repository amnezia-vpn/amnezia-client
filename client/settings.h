#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QSettings>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "core/defs.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;

class QSettings;

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);

    ServerCredentials defaultServerCredentials() const;
    ServerCredentials serverCredentials(int index) const;
    //void setServerCredentials(const ServerCredentials &credentials);

    QJsonArray serversArray() const { return QJsonDocument::fromJson(m_settings.value("Servers/serversList").toByteArray()).array(); }
    void setServersArray(const QJsonArray &servers) { m_settings.setValue("Servers/serversList", QJsonDocument(servers).toJson()); }

    // Servers section
    int serversCount() const;
    QJsonObject server(int index) const;
    void addServer(const QJsonObject &server);
    void removeServer(int index);
    bool editServer(int index, const QJsonObject &server);

    int defaultServerIndex() const { return m_settings.value("Servers/defaultServerIndex", 0).toInt(); }
    void setDefaultServer(int index) { m_settings.setValue("Servers/defaultServerIndex", index); }
    QJsonObject defaultServer() const { return server(defaultServerIndex()); }

    void setDefaultContainer(int serverIndex, DockerContainer container);
    DockerContainer defaultContainer(int serverIndex) const;
    QString defaultContainerName(int serverIndex) const;

    QMap<DockerContainer, QJsonObject> containers(int serverIndex) const;
    void setContainers(int serverIndex, const QMap<DockerContainer, QJsonObject> &containers);

    QJsonObject containerConfig(int serverIndex, DockerContainer container);
    void setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config);
    void removeContainerConfig(int serverIndex, DockerContainer container);

    QJsonObject protocolConfig(int serverIndex, DockerContainer container, Protocol proto);
    void setProtocolConfig(int serverIndex, DockerContainer container, Protocol proto, const QJsonObject &config);

    void clearLastConnectionConfig(int serverIndex, DockerContainer container, Protocol proto = Protocol::Any);

    bool haveAuthData(int serverIndex) const;
    QString nextAvailableServerName() const;

    // App settings section
    bool isAutoConnect() const { return m_settings.value("Conf/autoConnect", false).toBool(); }
    void setAutoConnect(bool enabled) { m_settings.setValue("Conf/autoConnect", enabled); }

    bool isStartMinimized() const { return m_settings.value("Conf/startMinimized", false).toBool(); }
    void setStartMinimized(bool enabled) { m_settings.setValue("Conf/startMinimized", enabled); }

    enum RouteMode {
        VpnAllSites,
        VpnOnlyForwardSites,
        VpnAllExceptSites
    };
    Q_ENUM (RouteMode)

    QString routeModeString(RouteMode mode) const;

    RouteMode routeMode() const  { return static_cast<RouteMode>(m_settings.value("Conf/routeMode", 0).toInt()); }
    void setRouteMode(RouteMode mode) { m_settings.setValue("Conf/routeMode", mode); }
    //    bool customRouting() const { return m_settings.value("Conf/customRouting", false).toBool(); }
//    void setCustomRouting(bool customRouting) { m_settings.setValue("Conf/customRouting", customRouting); }

//    // list of sites to pass blocking added by user
//    QStringList customSites() { return m_settings.value("Conf/customSites").toStringList(); }
//    void setCustomSites(const QStringList &customSites) { m_settings.setValue("Conf/customSites", customSites); }

//    // list of ips to pass blocking generated from customSites
//    QStringList customIps() { return m_settings.value("Conf/customIps").toStringList(); }
//    void setCustomIps(const QStringList &customIps) { m_settings.setValue("Conf/customIps", customIps); }


    QVariantMap vpnSites(RouteMode mode) const { return m_settings.value("Conf/" + routeModeString(mode)).toMap(); }
    void setVpnSites(RouteMode mode, const QVariantMap &sites) { m_settings.setValue("Conf/"+ routeModeString(mode), sites); m_settings.sync(); }
    void addVpnSite(RouteMode mode, const QString &site, const QString &ip= "");
    QStringList getVpnIps(RouteMode mode) const;
    void removeVpnSite(RouteMode mode, const QString &site);

    void addVpnIps(RouteMode mode, const QStringList &ip);
    void removeVpnSites(RouteMode mode, const QStringList &sites);


//    QVariantMap vpnForwardSites() const { return m_settings.value("Conf/vpnForwardSites").toMap(); }
//    void setVpnForwardSites(const QVariantMap &sites) { m_settings.setValue("Conf/vpnForwardSites", sites); }
//    void addVpnForwardSite(const QString &site, const QString &ip);
//    QStringList getVpnForwardIps() const;

//    QVariantMap vpnExceptSites() const { return m_settings.value("Conf/vpnExceptSites").toMap(); }
//    void setVpnExceptSites(const QVariantMap &sites) { m_settings.setValue("Conf/vpnExceptSites", sites); }
//    void addVpnExceptSite(const QString &site, const QString &ip);
//    QStringList getVpnExceptIps() const;

    QString primaryDns() const;
    QString secondaryDns() const;

    //QString primaryDns() const { return m_primaryDns; }
    void setPrimaryDns(const QString &primaryDns) { m_settings.setValue("Conf/primaryDns", primaryDns); }

    //QString secondaryDns() const { return m_secondaryDns; }
    void setSecondaryDns(const QString &secondaryDns) { m_settings.setValue("Conf/secondaryDns", secondaryDns); }

    static const char cloudFlareNs1[];
    static const char cloudFlareNs2[];

//    static constexpr char openNicNs5[] = "94.103.153.176";
//    static constexpr char openNicNs13[] = "144.76.103.143";


public:



private:
    QSettings m_settings;

};

#endif // SETTINGS_H
