#ifndef SECUREQSETTINGS_H
#define SECUREQSETTINGS_H

#include <QSettings>
#include <QObject>

class SecureQSettings : public QObject
{
public:
    explicit SecureQSettings(const QString &organization, const QString &application = QString(), QObject *parent = nullptr);

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    void remove(const QString &key);
    void sync();

    QByteArray backupAppConfig() const;
    void restoreAppConfig(const QByteArray &base64Cfg);

private:
    QSettings m_setting;
    bool encrypted {false};

    mutable QMap<QString, QVariant> m_cache;

    QStringList encryptedKeys; // encode only key listed here
};

#endif // SECUREQSETTINGS_H
