#include "secure_qsettings.h"
#include "secureformat.h"

#include <QDataStream>
#include <QDebug>

SecureQSettings::SecureQSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject{parent},
      m_setting(organization, application, parent),
      encryptedKeys({"Servers/serversList"})
{
    encrypted = m_setting.value("Conf/encrypted").toBool();

    // convert settings to encrypted
    if (! encrypted) {
        for (const QString &key : m_setting.allKeys()) {
            if (encryptedKeys.contains(key)) {
                const QVariant &val = value(key);
                setValue(key, val);
            }
        }
        m_setting.setValue("Conf/encrypted", true);
        m_setting.sync();
        encrypted = true;
    }
}

QVariant SecureQSettings::value(const QString &key, const QVariant &defaultValue) const
{
    if (m_cache.contains(key)) {
        return m_cache.value(key);
    }

    QVariant retVal;
    if (encrypted && encryptedKeys.contains(key)) {
        if (!m_setting.contains(key)) return defaultValue;

        QByteArray encryptedValue = m_setting.value(key).toByteArray();
        QByteArray decryptedValue = decryptText(encryptedValue);

        QDataStream ds(&decryptedValue, QIODevice::ReadOnly);
        ds >> retVal;
    }
    else {
        retVal = m_setting.value(key, defaultValue);
    }

    m_cache.insert(key, retVal);

    return retVal;
}

void SecureQSettings::setValue(const QString &key, const QVariant &value)
{
    QByteArray decryptedValue;
    {
        QDataStream ds(&decryptedValue, QIODevice::WriteOnly);
        ds << value;
    }

    QByteArray encryptedValue = encryptText(decryptedValue);
    m_setting.setValue(key, encryptedValue);
    m_cache.insert(key, value);

    sync();
}

void SecureQSettings::remove(const QString &key)
{
    m_setting.remove(key);
    m_cache.remove(key);

    sync();
}

void SecureQSettings::sync()
{
    m_setting.sync();
}

QByteArray SecureQSettings::backupAppConfig() const
{
    QMap<QString, QVariant> cfg;
    for (const QString &key : m_setting.allKeys()) {
        cfg.insert(key, value(key));
    }

    QByteArray ba;
    {
        QDataStream ds(&ba, QIODevice::WriteOnly);
        ds << cfg;
    }

    return ba.toBase64();
}

void SecureQSettings::restoreAppConfig(const QByteArray &base64Cfg)
{
    QByteArray ba = QByteArray::fromBase64(base64Cfg);
    QMap<QString, QVariant> cfg;

    {
        QDataStream ds(&ba, QIODevice::ReadOnly);
        ds >> cfg;
    }

    for (const QString &key : cfg.keys()) {
        setValue(key, cfg.value(key));
    }

    sync();
}


