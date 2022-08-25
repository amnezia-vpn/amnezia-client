#include "secure_qsettings.h"
#include "platforms/ios/MobileUtils.h"

#include <QDataStream>
#include <QDebug>
#include "utils.h"
#include <QRandomGenerator>
#include "QAead.h"
#include "QBlockCipher.h"

SecureQSettings::SecureQSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject{parent},
      m_settings(organization, application, parent),
      encryptedKeys({"Servers/serversList"})
{
    qDebug() << "SecureQSettings::SecureQSettings CTOR";

    bool encrypted = m_settings.value("Conf/encrypted").toBool();

    // convert settings to encrypted for if updated to >= 2.1.0
    if (encryptionRequired() && ! encrypted) {
        for (const QString &key : m_settings.allKeys()) {
            if (encryptedKeys.contains(key)) {
                const QVariant &val = value(key);
                setValue(key, val);
            }
        }
        m_settings.setValue("Conf/encrypted", true);
        m_settings.sync();
    }
}

QVariant SecureQSettings::value(const QString &key, const QVariant &defaultValue) const
{
    QMutexLocker locker(&mutex);

    if (m_cache.contains(key)) {
        return m_cache.value(key);
    }

    if (!m_settings.contains(key)) return defaultValue;

    QVariant retVal;

    // check if value is not encrypted, v. < 2.0.x
    retVal = m_settings.value(key);
    if (retVal.isValid()) {
        if (retVal.userType() == QVariant::ByteArray &&
                retVal.toByteArray().mid(0, magicString.size()) == magicString) {

            if (getEncKey().isEmpty() || getEncIv().isEmpty()) {
                qCritical() << "SecureQSettings::setValue Decryption requested, but key is empty";
                return {};
            }

            QByteArray encryptedValue = retVal.toByteArray().mid(magicString.size());

            QByteArray decryptedValue = decryptText(encryptedValue);
            QDataStream ds(&decryptedValue, QIODevice::ReadOnly);

            ds >> retVal;

            if (!retVal.isValid()) {
                qWarning() << "SecureQSettings::value settings decryption failed";
                retVal = QVariant();
            }
        }
    }
    else {
        qWarning() << "SecureQSettings::value invalid QVariant value";
        retVal = QVariant();
    }

    m_cache.insert(key, retVal);

    return retVal;
}

void SecureQSettings::setValue(const QString &key, const QVariant &value)
{
    QMutexLocker locker(&mutex);

    if (encryptionRequired() && encryptedKeys.contains(key)) {
        if (!getEncKey().isEmpty() && !getEncIv().isEmpty()) {
            QByteArray decryptedValue;
            {
                QDataStream ds(&decryptedValue, QIODevice::WriteOnly);
                ds << value;
            }

            QByteArray encryptedValue = encryptText(decryptedValue);
            m_settings.setValue(key, magicString + encryptedValue);
        }
        else {
            qCritical() << "SecureQSettings::setValue Encryption required, but key is empty";
            return;
        }

    }
    else {
        m_settings.setValue(key, value);
    }

    m_cache.insert(key, value);
    sync();
}

void SecureQSettings::remove(const QString &key)
{
    QMutexLocker locker(&mutex);

    m_settings.remove(key);
    m_cache.remove(key);

    sync();
}

void SecureQSettings::sync()
{
    m_settings.sync();
}

QByteArray SecureQSettings::backupAppConfig() const
{
    QMap<QString, QVariant> cfg;
    for (const QString &key : m_settings.allKeys()) {
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


QByteArray SecureQSettings::encryptText(const QByteArray& value) const
{
    QSimpleCrypto::QBlockCipher cipher;
    return cipher.encryptAesBlockCipher(value, getEncKey(), getEncIv());
}

QByteArray SecureQSettings::decryptText(const QByteArray& ba) const
{
    QSimpleCrypto::QBlockCipher cipher;
    return cipher.decryptAesBlockCipher(ba, getEncKey(), getEncIv());
}

bool SecureQSettings::encryptionRequired() const
{
#if defined Q_OS_IOS // || defined Q_OS_ANDROID
    return true;
#endif

    return false;
}

QByteArray SecureQSettings::getEncKey() const
{
    // load keys from system key storage
    m_key = MobileUtils::readFromKeychain(settingsKeyTag);

    if (m_key.isEmpty()) {
        // Create new key
        QSimpleCrypto::QBlockCipher cipher;
        QByteArray key = cipher.generateSecureRandomBytes(32);
        if (key.isEmpty()) {
            qCritical() << "SecureQSettings::getEncKey Unable to generate new enc key";
        }

        MobileUtils::writeToKeychain(settingsKeyTag, key);

        // check
        m_key = MobileUtils::readFromKeychain(settingsKeyTag);
        if (key != m_key) {
            qCritical() << "SecureQSettings::getEncKey Unable to store key in keychain" << key.size() << m_key.size();
            return {};
        }
    }

    return m_key;
}

QByteArray SecureQSettings::getEncIv() const
{
    // load keys from system key storage
    m_iv = MobileUtils::readFromKeychain(settingsIvTag);

    if (m_iv.isEmpty()) {
        // Create new IV
        QSimpleCrypto::QBlockCipher cipher;
        QByteArray iv = cipher.generateSecureRandomBytes(32);
        if (iv.isEmpty()) {
            qCritical() << "SecureQSettings::getEncIv Unable to generate new enc IV";
        }
        MobileUtils::writeToKeychain(settingsIvTag, iv);

        // check
        m_iv = MobileUtils::readFromKeychain(settingsIvTag);
        if (iv != m_iv) {
            qCritical() << "SecureQSettings::getEncIv Unable to store IV in keychain" << iv.size() << m_iv.size();
            return {};
        }
    }

    return m_iv;
}


