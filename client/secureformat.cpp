#include "secureformat.h"

#include <QTextStream>
#include <QVariantMap>
#include <QDebug>

#include "openssl/evp.h"

void handleErrors() {
    qDebug() << "handleErrors";
}

int generate_key_and_iv(unsigned char *iv, unsigned char *key) {
//    unsigned char key[32];
//    unsigned char iv[16];
//    EVP_BytesToKey(EVP_aes_256_gcm(), EVP_md5(),
//                   NULL,
//                   key_file_buf, key_size, 1, // const unsigned char *data, int datal, int count,
//                   key, iv);
    return 0;
}

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
        handleErrors();

    /*
     * Set IV length if default 12 bytes (96 bits) is not appropriate
     */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        handleErrors();

    /* Initialise key and IV */
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *key,
                unsigned char *iv, int iv_len,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. */
    if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
        handleErrors();

    /* Set IV length. Not necessary if this is 12 bytes (96 bits) */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        handleErrors();

    /* Initialise key and IV */
    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0) {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    } else {
        /* Verify failed */
        return -1;
    }
}

unsigned char gcmkey[] = "12345qwerty";
unsigned char iv[] = "000000000000";

QByteArray encryptText(const QByteArray& value) {
    int plainTextSize = value.size();
    unsigned char* plainText = new unsigned char[plainTextSize];
    std::memcpy(plainText, value.constData(), plainTextSize);

    unsigned char chipherText[UINT16_MAX];
    int chipherTextSize = gcm_encrypt(plainText, plainTextSize,
                                      gcmkey,
                                      iv, 12,
                                      chipherText);
    delete[] plainText;
    return QByteArray::fromRawData((const char *)chipherText, chipherTextSize);
}

QByteArray decryptText(const QByteArray& qEncryptArray) {
    unsigned char decryptPlainText[UINT16_MAX];
    gcm_decrypt((unsigned char*)qEncryptArray.data(), qEncryptArray.size(),
                gcmkey,
                iv, 12,
                decryptPlainText);
    return QByteArray::fromRawData((const char *)decryptPlainText, qEncryptArray.size());
}

SecureFormat::SecureFormat()
{
    m_format = QSettings::registerFormat("sconf",
                                         readSecureFile,
                                         writeSecureFile);
    qDebug() << "SecureFormat" << m_format;
}

bool SecureFormat::readSecureFile(QIODevice& device, QSettings::SettingsMap& map) {
    if (!device.isOpen()) {
        return false;
    }

    QTextStream inStream(&device);
    while (!inStream.atEnd()) {
        QString line = inStream.readLine();

        QStringList keyValue = line.split("<=>");
        QString key = keyValue.first();
        QString value = decryptText(keyValue.last().toUtf8());
        map.insert(key, value);

        qDebug() << "SecureFormat::readSecureFile: " << key << "<=>" << value;
    }

    return true;
}

bool SecureFormat::writeSecureFile(QIODevice& device, const QSettings::SettingsMap& map) {
//    if (!device.isOpen()) {
//        return false;
//    }

//    QTextStream outStream(&device);
//    auto keys = map.keys();
//    for (auto key : keys) {
//        QString value = map.value(key).toString();
//        QByteArray qEncryptArray = encryptText(value);
//        outStream << key << "<=>" << qEncryptArray << "\n";

//        qDebug() << "SecureFormat::writeSecureFile: " << key << "<=>" << qEncryptArray;
//    }

    return true;
}

void SecureFormat::chiperSettings(const QSettings &oldSetting, QSettings &newSetting) {
//    QVariantMap keysValuesPairs;
//    QStringList keys = oldSetting.allKeys();
//    QStringListIterator it(keys);
//    while ( it.hasNext() ) {
//        QString currentKey = it.next();
//        keysValuesPairs.insert(currentKey, oldSetting.value(currentKey));
//    }

//    for (const QString& key : keys) {
//        QString value = keysValuesPairs.value(key).toString();
//        QByteArray qEncryptArray = encryptText(value);

//        newSetting.setValue(key, qEncryptArray);
//    }

//    newSetting.sync();
}

const QSettings::Format& SecureFormat::format() const{
    return m_format;
}
