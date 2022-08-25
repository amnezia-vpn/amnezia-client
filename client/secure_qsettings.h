#ifndef SECUREQSETTINGS_H
#define SECUREQSETTINGS_H

#include <QSettings>
#include <QObject>
#include <QMutex>
#include <QMutexLocker>


constexpr const char* settingsKeyTag = "settingsKeyTag";
constexpr const char* settingsIvTag = "settingsIvTag";


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

    QByteArray encryptText(const QByteArray &value) const;
    QByteArray decryptText(const QByteArray& ba) const;

    bool encryptionRequired() const;

    QByteArray getEncKey() const;
    QByteArray getEncIv() const;
private:
    QSettings m_settings;

    mutable QMap<QString, QVariant> m_cache;

    QStringList encryptedKeys; // encode only key listed here

    mutable QByteArray m_key;
    mutable QByteArray m_iv;

    const QByteArray magicString { "EncData" }; // Magic keyword used for mark encrypted QByteArray

    mutable QMutex mutex;
};

#endif // SECUREQSETTINGS_H
