#ifndef SECUREQSETTINGS_H
#define SECUREQSETTINGS_H

#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QSettings>

#include "keychain.h"

class SecureQSettings : public QObject
{
    Q_OBJECT

public:
    explicit SecureQSettings(const QString &organization, const QString &application = QString(),
                             QObject *parent = nullptr);

    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    void remove(const QString &key);
    void sync();

    QByteArray backupAppConfig() const;
    bool restoreAppConfig(const QByteArray &json);

    QByteArray encryptText(const QByteArray &value) const;
    QByteArray decryptText(const QByteArray &ba) const;

    bool encryptionRequired() const;

    QByteArray getEncKey() const;
    QByteArray getEncIv() const;

    static QByteArray getSecTag(const QString &tag);
    static void setSecTag(const QString &tag, const QByteArray &data);

    void clearSettings();

private:
    QSettings m_settings;

    mutable QHash<QString, QVariant> m_cache;

    QStringList encryptedKeys; // encode only key listed here
    // only this fields need for backup
    QStringList m_fieldsToBackup = {
        "Conf/", "Servers/",
    };

    mutable QByteArray m_key;
    mutable QByteArray m_iv;

    const QByteArray magicString { "EncData" }; // Magic keyword used for mark encrypted QByteArray

    mutable QMutex mutex;
};

#endif // SECUREQSETTINGS_H
