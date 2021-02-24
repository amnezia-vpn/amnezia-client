#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QSettings>

#include "core/defs.h"

using namespace amnezia;

class QSettings;

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);

    QString userName() const { return m_settings.value("Server/userName", QString()).toString(); }
    void setUserName(const QString& login) { m_settings.setValue("Server/userName", login); }

    QString password() const { return m_settings.value("Server/password", QString()).toString(); }
    void setPassword(const QString& password) { m_settings.setValue("Server/password", password); }

    QString serverName() const { return m_settings.value("Server/serverName", QString()).toString(); }
    void setServerName(const QString& serverName) { m_settings.setValue("Server/serverName", serverName); }

    int serverPort() const { return m_settings.value("Server/serverPort", 22).toInt(); }
    void setServerPort(int serverPort = 22) { m_settings.setValue("Server/serverPort", serverPort); }

    ServerCredentials serverCredentials();
    void setServerCredentials(const ServerCredentials &credentials);
    bool haveAuthData() const;

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

    QString primaryDns() const { return m_settings.value("Conf/primaryDns", cloudFlareNs1()).toString(); }
    QString secondaryDns() const { return m_settings.value("Conf/secondaryDns", cloudFlareNs2()).toString(); }

    //QString primaryDns() const { return m_primaryDns; }
    void setPrimaryDns(const QString &primaryDns) { m_settings.setValue("Conf/primaryDns", primaryDns); }

    //QString secondaryDns() const { return m_secondaryDns; }
    void setSecondaryDns(const QString &secondaryDns) { m_settings.setValue("Conf/secondaryDns", secondaryDns); }

    QString cloudFlareNs1() const { return "1.1.1.1"; }
    QString cloudFlareNs2() const { return "1.0.0.1"; }

    QString openNicNs5() const { return "94.103.153.176"; }
    QString openNicNs13() const { return "144.76.103.143"; }

private:
    QSettings m_settings;
};

#endif // SETTINGS_H
