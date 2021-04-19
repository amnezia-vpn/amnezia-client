#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QSettings>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "core/defs.h"

using namespace amnezia;

class QSettings;

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);


//    QString userName() const { return m_settings.value("Server/userName", QString()).toString(); }
//    void setUserName(const QString& login) { m_settings.setValue("Server/userName", login); }

//    QString password() const { return m_settings.value("Server/password", QString()).toString(); }
//    void setPassword(const QString& password) { m_settings.setValue("Server/password", password); }

//    QString serverName() const { return m_settings.value("Server/serverName", QString()).toString(); }
//    void setServerName(const QString& serverName) { m_settings.setValue("Server/serverName", serverName); }

//    int serverPort() const { return m_settings.value("Server/serverPort", 22).toInt(); }
//    void setServerPort(int serverPort = 22) { m_settings.setValue("Server/serverPort", serverPort); }

    ServerCredentials defaultServerCredentials() const;
    //void setServerCredentials(const ServerCredentials &credentials);

    QJsonArray serversArray() const {return QJsonDocument::fromJson(m_settings.value("Servers/serversList").toByteArray()).array(); }
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

    void setDefaultContainer(int serverIndex, DockerContainer container );
    DockerContainer defaultContainer(int serverIndex) const;
    QString defaultContainerName(int serverIndex) const;

    bool haveAuthData() const;

    // App settings section
    bool isAutoConnect() const { return m_settings.value("Conf/autoConnect", QString()).toBool(); }
    void setAutoConnect(bool enabled) { m_settings.setValue("Conf/autoConnect", enabled); }

    bool customRouting() const { return m_settings.value("Conf/customRouting", false).toBool(); }
    void setCustomRouting(bool customRouting) { m_settings.setValue("Conf/customRouting", customRouting); }

    // list of sites to pass blocking added by user
    QStringList customSites() { return m_settings.value("Conf/customSites").toStringList(); }
    void setCustomSites(const QStringList &customSites) { m_settings.setValue("Conf/customSites", customSites); }

    // list of ips to pass blocking generated from customSites
    QStringList customIps() { return m_settings.value("Conf/customIps").toStringList(); }
    void setCustomIps(const QStringList &customIps) { m_settings.setValue("Conf/customIps", customIps); }

    QString primaryDns() const { return m_settings.value("Conf/primaryDns", cloudFlareNs1).toString(); }
    QString secondaryDns() const { return m_settings.value("Conf/secondaryDns", cloudFlareNs2).toString(); }

    //QString primaryDns() const { return m_primaryDns; }
    void setPrimaryDns(const QString &primaryDns) { m_settings.setValue("Conf/primaryDns", primaryDns); }

    //QString secondaryDns() const { return m_secondaryDns; }
    void setSecondaryDns(const QString &secondaryDns) { m_settings.setValue("Conf/secondaryDns", secondaryDns); }

    static constexpr char cloudFlareNs1[] = "1.1.1.1";
    static constexpr char cloudFlareNs2[] = "1.0.0.1";

    static constexpr char openNicNs5[] = "94.103.153.176";
    static constexpr char openNicNs13[] = "144.76.103.143";


public:
    // Json strings
    static constexpr char hostNameString[] = "hostName";
    static constexpr char userNameString[] = "userName";
    static constexpr char passwordString[] = "password";
    static constexpr char portString[] = "port";
    static constexpr char descriptionString[] = "description";

    static constexpr char defaultContainerString[] = "defaultContainer";


private:
    QSettings m_settings;



};

#endif // SETTINGS_H
