#include "secure_qsettings.h"

#include "QAead.h"
#include "QBlockCipher.h"
#include "utilities.h"
#include <QDataStream>
#include <QDebug>
#include <QEventLoop>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QTimer>

using namespace QKeychain;

namespace {
constexpr auto settingsKeyTag{"settingsKeyTag"};
constexpr auto settingsIvTag{"settingsIvTag"};
constexpr auto keyChainName{"AmneziaVPN-Keychain"};
}

SecureQSettings::SecureQSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject { parent }, m_settings(organization, application, parent), encryptedKeys({ "Servers/serversList" })
{
    bool encrypted = m_settings.value("Conf/encrypted").toBool();

    // convert settings to encrypted for if updated to >= 2.1.0
    if (encryptionRequired() && !encrypted) {
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

    if (!m_settings.contains(key))
        return defaultValue;

    QVariant retVal;

    // check if value is not encrypted, v. < 2.0.x
    retVal = m_settings.value(key);
    if (retVal.isValid()) {
        if (retVal.userType() == QMetaType::QByteArray && retVal.toByteArray().mid(0, magicString.size()) == magicString) {

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
    } else {
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
        } else {
            qCritical() << "SecureQSettings::setValue Encryption required, but key is empty";
            return;
        }

    } else {
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
    QJsonObject cfg;

    const auto needToBackup = [this](const auto &key) {
      for (const auto &item : m_fieldsToBackup)
      {
        if (key == "Conf/installationUuid")
        {
          return false;
        }

        if (key.startsWith(item))
        {
            return true;
        }
      }

      return false;
    };

    for (const QString &key : m_settings.allKeys()) {

        if (!needToBackup(key))
        {
            continue;
        }

        cfg.insert(key, QJsonValue::fromVariant(value(key)));
    }

    return QJsonDocument(cfg).toJson();
}

bool SecureQSettings::restoreAppConfig(const QByteArray &json)
{
    QJsonObject cfg = QJsonDocument::fromJson(json).object();
    if (cfg.isEmpty())
        return false;

    for (const QString &key : cfg.keys()) {
        if (key == "Conf/installationUuid") {
            continue;
        }

        setValue(key, cfg.value(key).toVariant());
    }

    sync();
    return true;
}

QByteArray SecureQSettings::encryptText(const QByteArray &value) const
{
    QSimpleCrypto::QBlockCipher cipher;
    QByteArray result;
    try {
        result = cipher.encryptAesBlockCipher(value, getEncKey(), getEncIv());
    } catch (...) { // todo change error handling in QSimpleCrypto?
        qCritical() << "error when encrypting the settings value";
    }
    return result;
}

QByteArray SecureQSettings::decryptText(const QByteArray &ba) const
{
    QSimpleCrypto::QBlockCipher cipher;
    QByteArray result;
    try {
        result = cipher.decryptAesBlockCipher(ba, getEncKey(), getEncIv());
    } catch (...) { // todo change error handling in QSimpleCrypto?
        qCritical() << "error when decrypting the settings value";
    }
    return result;
}

bool SecureQSettings::encryptionRequired() const
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    // QtKeyChain failing on Linux
    return false;
#endif
    return true;
}

QByteArray SecureQSettings::getEncKey() const
{
    // load keys from system key storage
    m_key = getSecTag(settingsKeyTag);

    if (m_key.isEmpty()) {
        // Create new key
        QSimpleCrypto::QBlockCipher cipher;
        QByteArray key = cipher.generatePrivateSalt(32);
        if (key.isEmpty()) {
            qCritical() << "SecureQSettings::getEncKey Unable to generate new enc key";
        }

        setSecTag(settingsKeyTag, key);

        // check
        m_key = getSecTag(settingsKeyTag);
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
    m_iv = getSecTag(settingsIvTag);

    if (m_iv.isEmpty()) {
        // Create new IV
        QSimpleCrypto::QBlockCipher cipher;
        QByteArray iv = cipher.generatePrivateSalt(32);
        if (iv.isEmpty()) {
            qCritical() << "SecureQSettings::getEncIv Unable to generate new enc IV";
        }
        setSecTag(settingsIvTag, iv);

        // check
        m_iv = getSecTag(settingsIvTag);
        if (iv != m_iv) {
            qCritical() << "SecureQSettings::getEncIv Unable to store IV in keychain" << iv.size() << m_iv.size();
            return {};
        }
    }

    return m_iv;
}

QByteArray SecureQSettings::getSecTag(const QString &tag)
{
    auto job = QSharedPointer<ReadPasswordJob>(new ReadPasswordJob(keyChainName), &QObject::deleteLater);
    job->setAutoDelete(false);
    job->setKey(tag);
    QEventLoop loop;
    job->connect(job.data(), &ReadPasswordJob::finished, job.data(), [&loop]() { loop.quit(); });
    job->start();
    loop.exec();

    if (job->error()) {
        qCritical() << "SecureQSettings::getSecTag Error:" << job->errorString();
    }

    return job->binaryData();
}

void SecureQSettings::setSecTag(const QString &tag, const QByteArray &data)
{
    auto job = QSharedPointer<WritePasswordJob>(new WritePasswordJob(keyChainName), &QObject::deleteLater);
    job->setAutoDelete(false);
    job->setKey(tag);
    job->setBinaryData(data);
    QEventLoop loop;
    QTimer::singleShot(1000, &loop, SLOT(quit()));
    job->connect(job.data(), &WritePasswordJob::finished, job.data(), [&loop]() { loop.quit(); });
    job->start();
    loop.exec();

    if (job->error()) {
        qCritical() << "SecureQSettings::setSecTag Error:" << job->errorString();
    }
}

void SecureQSettings::clearSettings()
{
    QMutexLocker locker(&mutex);
    m_settings.clear();
    m_cache.clear();
    sync();
}
