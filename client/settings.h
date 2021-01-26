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

    void read();
    void save();

    void setUserName(const QString& login);
    void setPassword(const QString& password);
    void setServerName(const QString& serverName);
    void setServerPort(int serverPort = 22);
    void setServerCredentials(const ServerCredentials &credentials);

    QString userName() const { return m_userName; }
    QString password() const { return m_password; }
    QString serverName() const { return m_serverName; }
    int serverPort() const { return m_serverPort; }
    ServerCredentials serverCredentials();


    bool haveAuthData() const;


    // list of sites to pass blocking added by user
    QStringList customSites() { return m_settings.value("customSites").toStringList(); }
    void setCustomSites(const QStringList &customSites) { m_settings.setValue("customSites", customSites); }

    // list of ips to pass blocking generated from customSites
    QStringList customIps() { return m_settings.value("customIps").toStringList(); }
    void setCustomIps(const QStringList &customIps) { m_settings.setValue("customIps", customIps); }


protected:
    QSettings m_settings;
    QString m_userName;
    QString m_password;
    QString m_serverName;
    int m_serverPort;
};

#endif // SETTINGS_H
