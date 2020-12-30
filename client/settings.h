#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>

class QSettings;

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);

    void read();
    void save();

    void setLogin(const QString& login);
    void setPassword(const QString& password);
    void setServerName(const QString& serverName);

    QString login() const;
    QString password() const;
    QString serverName() const;

    bool haveAuthData() const;

protected:
    QSettings* m_settings;
    QString m_login;
    QString m_password;
    QString m_serverName;
};

#endif // SETTINGS_H
