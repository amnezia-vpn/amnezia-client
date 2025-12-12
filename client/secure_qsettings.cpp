#include "secure_qsettings.h"

#include "../client/3rd/QSimpleCrypto/src/include/QAead.h"
#include "../client/3rd/QSimpleCrypto/src/include/QBlockCipher.h"
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
#include <QFile>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

using namespace QKeychain;

namespace {
    constexpr const char *settingsKeyTag = "settingsKeyTag";
    constexpr const char *settingsIvTag = "settingsIvTag";
    constexpr const char *keyChainName = "AmneziaVPN-Keychain";

    constexpr int SALT_LEN = 16;
    constexpr int IV_LEN = 16;
    constexpr int KEY_LEN = 32;
    constexpr int PBKDF2_ITER = 100000;
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

static QString opensslErrString()
{
    unsigned long e = ERR_get_error();
    if (!e)
        return QStringLiteral("Unknown OpenSSL error");
    char buf[256];
    ERR_error_string_n(e, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

static bool deriveKey(const QByteArray &password, const QByteArray &salt, QByteArray &outKey, QString *err)
{
    outKey.resize(KEY_LEN);
    const unsigned char *pw = reinterpret_cast<const unsigned char *>(password.constData());
    const unsigned char *s = reinterpret_cast<const unsigned char *>(salt.constData());
    int ok = PKCS5_PBKDF2_HMAC(reinterpret_cast<const char *>(pw), password.size(), s, salt.size(), PBKDF2_ITER,
                               EVP_sha256(), KEY_LEN, reinterpret_cast<unsigned char *>(outKey.data()));
    if (!ok) {
        if (err)
            *err = opensslErrString();
    }
    return ok == 1;
}

static bool aesCrypt(const QByteArray &in, const QByteArray &key, const QByteArray &iv, QByteArray &out, bool encrypt,
                     QString *err)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (err)
            *err = "EVP_CIPHER_CTX_new failed";
        return false;
    }
    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    if (1
        != EVP_CipherInit_ex(ctx, cipher, nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                             reinterpret_cast<const unsigned char *>(iv.constData()), encrypt ? 1 : 0)) {
        if (err)
            *err = opensslErrString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    out.clear();
    out.resize(in.size() + EVP_CIPHER_block_size(cipher));
    int outlen1 = 0;
    if (1
        != EVP_CipherUpdate(ctx, reinterpret_cast<unsigned char *>(out.data()), &outlen1,
                            reinterpret_cast<const unsigned char *>(in.constData()), in.size())) {
        if (err)
            *err = opensslErrString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    int outlen2 = 0;
    if (1 != EVP_CipherFinal_ex(ctx, reinterpret_cast<unsigned char *>(out.data()) + outlen1, &outlen2)) {
        if (err)
            *err = opensslErrString();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    out.resize(outlen1 + outlen2);
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool SecureQSettings::encryptFile(const QString &filePath, const QString &password, QString *error) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot open file for read: %1").arg(f.errorString());
        return false;
    }
    QByteArray plain = f.readAll();
    f.close();

    if (plain.startsWith(magicString)) {
        if (error)
            *error = QStringLiteral("File already encrypted (magic found)");
        return false;
    }

    QByteArray salt(SALT_LEN, 0);
    QByteArray iv(IV_LEN, 0);
    QByteArray key;
    QByteArray cipher;
    QByteArray out;

    if (1 != RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), SALT_LEN)
        || 1 != RAND_bytes(reinterpret_cast<unsigned char *>(iv.data()), IV_LEN)) {
        if (error)
            *error = opensslErrString();
        return false;
    }

    if (!deriveKey(password.toUtf8(), salt, key, error))
        return false;

    if (!aesCrypt(plain, key, iv, cipher, true, error))
        return false;

    out.reserve(magicString.size() + SALT_LEN + IV_LEN + cipher.size());
    out += magicString;
    out += salt;
    out += iv;
    out += cipher;

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("Cannot open file for write: %1").arg(f.errorString());
        return false;
    }
    qint64 written = f.write(out);
    f.close();
    if (written != out.size()) {
        if (error)
            *error = QStringLiteral("Write failed or incomplete");
        return false;
    }
    return true;
}

bool SecureQSettings::decryptFile(const QString &filePath, const QString &password, QString *error) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot open file for read: %1").arg(f.errorString());
        return false;
    }
    QByteArray blob = f.readAll();
    f.close();

    if (!blob.startsWith(magicString)) {
        if (error)
            *error = QStringLiteral("File is not recognized as encrypted (magic missing)");
        return false;
    }

    int pos = magicString.size();
    if (blob.size() < pos + SALT_LEN + IV_LEN) {
        if (error)
            *error = QStringLiteral("Encrypted file too small / corrupted");
        return false;
    }

    QByteArray salt = blob.mid(pos, SALT_LEN);
    pos += SALT_LEN;
    QByteArray iv = blob.mid(pos, IV_LEN);
    pos += IV_LEN;
    QByteArray cipher = blob.mid(pos);
    QByteArray key;
    QByteArray plain;

    if (!deriveKey(password.toUtf8(), salt, key, error))
        return false;

    if (!aesCrypt(cipher, key, iv, plain, false, error))
        return false;

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("Cannot open file for write: %1").arg(f.errorString());
        return false;
    }
    qint64 written = f.write(plain);
    f.close();
    if (written != plain.size()) {
        if (error)
            *error = QStringLiteral("Write failed or incomplete");
        return false;
    }
    return true;
}

void SecureQSettings::clearSettings()
{
    QMutexLocker locker(&mutex);
    m_settings.clear();
    m_cache.clear();
    sync();
}
