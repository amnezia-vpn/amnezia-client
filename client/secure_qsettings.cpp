#include "secure_qsettings.h"
#include "encryption_helper.h"
#include "platforms/ios/MobileUtils.h"

#include <QDataStream>
#include <QDebug>

SecureQSettings::SecureQSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject{parent},
      m_setting(organization, application, parent),
      encryptedKeys({"Servers/serversList"})
{
    // load keys from system key storage
#ifdef Q_OS_IOS
    key = QByteArray::fromBase64(MobileUtils::readFromKeychain(settingsKeyTag).toUtf8());
    iv = QByteArray::fromBase64(MobileUtils::readFromKeychain(settingsIvTag).toUtf8());
#endif
    key = "12345qwerty00000";
    iv = "000000000000000";

    bool encrypted = m_setting.value("Conf/encrypted").toBool();

    // convert settings to encrypted
    if (encryptionRequired() && ! encrypted) {
        for (const QString &key : m_setting.allKeys()) {
            if (encryptedKeys.contains(key)) {
                const QVariant &val = value(key);
                setValue(key, val);
            }
        }
        m_setting.setValue("Conf/encrypted", true);
        m_setting.sync();
    }
}

QVariant SecureQSettings::value(const QString &key, const QVariant &defaultValue) const
{
    if (m_cache.contains(key)) {
        return m_cache.value(key);
    }

    QVariant retVal;
    if (encryptionRequired() && encryptedKeys.contains(key)) {
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
    if (encryptionRequired() && encryptedKeys.contains(key)) {
        QByteArray decryptedValue;
        {
            QDataStream ds(&decryptedValue, QIODevice::WriteOnly);
            ds << value;
        }

        QByteArray encryptedValue = encryptText(decryptedValue);
        m_setting.setValue(key, encryptedValue);
    }
    else {
        m_setting.setValue(key, value);
    }

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


QByteArray SecureQSettings::encryptText(const QByteArray& value) const {
    char cipherText[UINT16_MAX];
    int cipherTextSize = gcm_encrypt(value.constData(), value.size(),
        key.constData(), iv.constData(), iv_len, cipherText);

    return QByteArray::fromRawData((const char *)cipherText, cipherTextSize);
}

QByteArray SecureQSettings::decryptText(const QByteArray& ba) const {
    char decryptPlainText[UINT16_MAX];
    gcm_decrypt(ba.data(), ba.size(),
        key.constData(), iv.constData(), iv_len, decryptPlainText);

    return QByteArray::fromRawData(decryptPlainText, ba.size());
}

bool SecureQSettings::encryptionRequired() const
{
#if defined Q_OS_ANDROID || defined Q_OS_IOS
    return true;
#endif

    return false;
}


