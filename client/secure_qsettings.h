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
    void sync() { m_setting.sync(); }
    void remove(const QString &key) { m_setting.remove(key); }

private:
    QSettings m_setting;
    bool encrypted {false};
};

#endif // SECUREQSETTINGS_H
