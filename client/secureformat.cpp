#include "secureformat.h"

#include <QTextStream>
#include <QDebug>

#include "openssl/evp.h"

void handleErrors() {
    qDebug() << "handleErrors";
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

SecureFormat::SecureFormat()
{
    m_format = QSettings::registerFormat("plist",
                                         readSecureFile,
                                         writeSecureFile);
    qDebug() << "SecureFormat" << m_format;
    
    unsigned char plainText[] = "Hello world!";
    qDebug("%s", plainText);
    unsigned char key[] = "12345qwerty";
    unsigned char iv[] = "000000000000";
    unsigned char chipherText[1024];
    unsigned char decryptPlainText[1024];
    gcm_encrypt(plainText, std::strlen((const char *)plainText),
                key,
                iv, 12,
                chipherText);
    qDebug("%s", chipherText);
    
    gcm_decrypt(chipherText, std::strlen((const char *)chipherText),
                key,
                iv, 12,
                decryptPlainText);
    qDebug("%s", decryptPlainText);
}

bool SecureFormat::readSecureFile(QIODevice& device, QSettings::SettingsMap& map) {
    if (!device.isOpen()) {
        return false;
    }

    QTextStream inStream(&device);
    while (!inStream.atEnd()) {
        QString line = inStream.readLine();
        qDebug() << "SecureFormat::readSecureFile" << line;
    }

    return true;
}

bool SecureFormat::writeSecureFile(QIODevice& device, const QSettings::SettingsMap& map) {
    if (!device.isOpen()) {
        return false;
    }

    QTextStream outStream(&device);

    return true;
}

const QSettings::Format& SecureFormat::format() const{
    return m_format;
}
