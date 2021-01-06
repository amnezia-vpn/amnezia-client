#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>

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
    void setServerPort(int serverPort);
    void setServerCredentials(const ServerCredentials &credentials);

    QString userName() const { return m_userName; }
    QString password() const { return m_password; }
    QString serverName() const { return m_serverName; }
    int serverPort() const { return m_serverPort; }
    ServerCredentials serverCredentials();


    bool haveAuthData() const;

protected:
    QSettings* m_settings;
    QString m_userName;
    QString m_password;
    QString m_serverName;
    int m_serverPort;
};

#endif // SETTINGS_H
