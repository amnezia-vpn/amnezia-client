#include "protection.h"
#include <QFile>
#include <QCryptographicHash>
#include <QDebug>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>

static constexpr int KEY_SIZE = 32;
static constexpr int SALT_SIZE = 16;
static constexpr int IV_SIZE = 16;
static constexpr int ITERATIONS = 100000;

Protector::Protector(QObject *parent) : QObject(parent)
{
}

QByteArray Protector::generateSalt()
{
    QByteArray salt(SALT_SIZE, 0);
    RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), SALT_SIZE);
    return salt;
}

QByteArray Protector::deriveKey(const QString &password, const QByteArray &salt)
{
    QByteArray key(KEY_SIZE, 0);
    if (!PKCS5_PBKDF2_HMAC(password.toUtf8().constData(), password.size(),
                           reinterpret_cast<const unsigned char *>(salt.constData()), salt.size(), ITERATIONS,
                           EVP_sha256(), KEY_SIZE, reinterpret_cast<unsigned char *>(key.data()))) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }
    return key;
}

void Protector::encryptFile(const QString &filePath, const QString &password)
{
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Cannot open file for reading");

        QByteArray plain = file.readAll();
        file.close();

        QByteArray salt = generateSalt();
        QByteArray iv(IV_SIZE, 0);
        RAND_bytes(reinterpret_cast<unsigned char *>(iv.data()), IV_SIZE);
        QByteArray key = deriveKey(password, salt);

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        QByteArray encrypted(plain.size() + EVP_MAX_BLOCK_LENGTH, 0);
        int len = 0, total = 0;

        if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                                reinterpret_cast<const unsigned char *>(iv.constData())))
            throw std::runtime_error("EncryptInit failed");

        if (!EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char *>(encrypted.data()), &len,
                               reinterpret_cast<const unsigned char *>(plain.constData()), plain.size()))
            throw std::runtime_error("EncryptUpdate failed");

        total = len;

        if (!EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(encrypted.data()) + len, &len))
            throw std::runtime_error("EncryptFinal failed");

        total += len;
        EVP_CIPHER_CTX_free(ctx);
        encrypted.truncate(total);

        QByteArray finalData = salt + iv + encrypted;

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            throw std::runtime_error("Cannot open file for writing");

        file.write(finalData);
        file.close();

        qInfo() << "File encrypted successfully:" << filePath;
    } catch (const std::exception &e) {
        qWarning() << "Encryption failed:" << e.what();
    }
}

void Protector::decryptFile(const QString &filePath, const QString &password)
{
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Cannot open file for reading");

        QByteArray data = file.readAll();
        file.close();

        if (data.size() < SALT_SIZE + IV_SIZE)
            throw std::runtime_error("Corrupted or invalid encrypted file");

        QByteArray salt = data.left(SALT_SIZE);
        QByteArray iv = data.mid(SALT_SIZE, IV_SIZE);
        QByteArray cipher = data.mid(SALT_SIZE + IV_SIZE);

        QByteArray key = deriveKey(password, salt);
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        QByteArray plain(cipher.size(), 0);
        int len = 0, total = 0;

        if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                                reinterpret_cast<const unsigned char *>(iv.constData())))
            throw std::runtime_error("DecryptInit failed");

        if (!EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char *>(plain.data()), &len,
                               reinterpret_cast<const unsigned char *>(cipher.constData()), cipher.size()))
            throw std::runtime_error("DecryptUpdate failed");

        total = len;

        if (!EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(plain.data()) + len, &len)) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Incorrect password or corrupted file");
        }

        total += len;
        EVP_CIPHER_CTX_free(ctx);
        plain.truncate(total);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            throw std::runtime_error("Cannot open file for writing");

        file.write(plain);
        file.close();

        qInfo() << "File decrypted successfully:" << filePath;
    } catch (const std::exception &e) {
        qWarning() << "Decryption failed:" << e.what();
    }
}
