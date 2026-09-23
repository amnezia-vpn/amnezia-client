#include "systemController.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace
{
    constexpr int SALT_LEN = 16;
    constexpr int IV_LEN = 12;
    constexpr int KEY_LEN = 32;
    constexpr int TAG_LEN = 16;
    constexpr int PBKDF2_ITER = 100000;

    const QByteArray magicString { "EncData" };

    static QString opensslErrString()
    {
        unsigned long e = ERR_get_error();

        if (!e)
            return QStringLiteral("Unknown OpenSSL error");

        char buf[256];
        ERR_error_string_n(e, buf, sizeof(buf));

        return QString::fromUtf8(buf);
    }

    static bool deriveKey(const QByteArray &password, const QByteArray &salt, QByteArray &outKey)
    {
        outKey.resize(KEY_LEN);

        const int ok = PKCS5_PBKDF2_HMAC(
                password.constData(), password.size(), reinterpret_cast<const unsigned char *>(salt.constData()),
                salt.size(), PBKDF2_ITER, EVP_sha256(), KEY_LEN, reinterpret_cast<unsigned char *>(outKey.data()));

        if (ok != 1) {
            qDebug() << "PBKDF2 failed:" << opensslErrString();
            outKey.clear();
            return false;
        }

        return true;
    }

    static bool aesCrypt(const QByteArray &in, const QByteArray &key, const QByteArray &iv, QByteArray &out,
                         QByteArray &tag, bool encrypt)
    {
        std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX *)> ctx { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };

        if (!ctx) {
            qDebug() << "EVP_CIPHER_CTX_new failed";
            return false;
        }

        const EVP_CIPHER *cipher = EVP_aes_256_gcm();
        if (EVP_CipherInit_ex(ctx.get(), cipher, nullptr, nullptr, nullptr, encrypt ? 1 : 0) != 1) {
            qDebug() << opensslErrString();
            return false;
        }

        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) != 1) {
            qDebug() << opensslErrString();
            return false;
        }

        if (EVP_CipherInit_ex(ctx.get(), nullptr, nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                              reinterpret_cast<const unsigned char *>(iv.constData()), -1)
            != 1) {
            qDebug() << opensslErrString();
            return false;
        }

        out.resize(in.size());

        int outLen = 0;
        if (EVP_CipherUpdate(ctx.get(), reinterpret_cast<unsigned char *>(out.data()), &outLen,
                             reinterpret_cast<const unsigned char *>(in.constData()), in.size())
            != 1) {
            qDebug() << opensslErrString();
            return false;
        }

        int finalLen = 0;
        if (encrypt) {
            if (EVP_CipherFinal_ex(ctx.get(), reinterpret_cast<unsigned char *>(out.data()) + outLen, &finalLen) != 1) {
                qDebug() << opensslErrString();
                return false;
            }

            out.resize(outLen + finalLen);
            tag.resize(TAG_LEN);

            if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag.data()) != 1) {
                qDebug() << opensslErrString();
                return false;
            }
        } else {
            if (tag.size() != TAG_LEN) {
                qDebug() << "Invalid GCM tag size:" << tag.size();
                return false;
            }

            if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, TAG_LEN, const_cast<char *>(tag.constData())) != 1) {
                qDebug() << opensslErrString();
                return false;
            }

            if (EVP_CipherFinal_ex(ctx.get(), reinterpret_cast<unsigned char *>(out.data()) + outLen, &finalLen) != 1) {
                qDebug() << "Authentication failed:" << opensslErrString();
                return false;
            }

            out.resize(outLen + finalLen);
        }

        return true;
    }
}

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
    #include <CoreFoundation/CoreFoundation.h>
#endif

SystemController::SystemController(QObject *parent)
    : QObject(parent)
{
}

bool SystemController::saveFile(const QString &fileName, const QString &data)
{
#if defined Q_OS_ANDROID
    AndroidController::instance()->saveFile(fileName, data);
    return true;
#endif
    return saveFile(fileName, data.toUtf8());
}

bool SystemController::saveFile(const QString &fileName, const QByteArray &data)
{
#if defined Q_OS_ANDROID
    AndroidController::instance()->saveFile(fileName, QString::fromUtf8(data));
    return true;
#endif

#ifdef Q_OS_IOS
    QUrl fileUrl = QDir::tempPath() + "/" + fileName;
    QFile file(fileUrl.toString());
#else
    QFile file(fileName);
#endif

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "SystemController::saveFile: cannot open" << fileName;
        return false;
    }
    if (file.write(data) != data.size()) {
        qWarning() << "SystemController::saveFile: write failed" << fileName;
        file.close();
        return false;
    }
    file.close();

#ifdef Q_OS_IOS
    QStringList filesToSend;
    filesToSend.append(fileUrl.toString());
    return IosController::Instance()->shareText(filesToSend);
#else
    QFileInfo fi(fileName);

#ifdef Q_OS_MAC
    const auto url = "file://" + fi.absoluteDir().absolutePath();
#else
    const auto url = fi.absoluteDir().absolutePath();
#endif

#ifndef MACOS_NE
    QDesktopServices::openUrl(url);
#endif
    return true;
#endif
}

bool SystemController::readFile(const QString &fileName, QByteArray &data)
{
#ifdef Q_OS_ANDROID
    int fd = AndroidController::instance()->getFd(fileName);
    if (fd == -1) return false;
    QFile file;
    if(!file.open(fd, QIODevice::ReadOnly)) return false;
    data = file.readAll();
    AndroidController::instance()->closeFd();
#else
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return false;
    data = file.readAll();
#endif
    return true;
}

bool SystemController::readFile(const QString &fileName, QString &data)
{
    QByteArray byteArray;
    if(!readFile(fileName, byteArray)) return false;
    data = byteArray;
    return true;
}

QByteArray SystemController::encryptData(const QByteArray &data, const QString &password, const QString &hint)
{
    QByteArray salt(SALT_LEN, Qt::Uninitialized);
    QByteArray iv(IV_LEN, Qt::Uninitialized);
    QByteArray key;
    QByteArray cipher;
    QByteArray tag;

    if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), SALT_LEN) != 1) {
        qDebug() << "Failed to generate salt:" << opensslErrString();
        return {};
    }

    if (RAND_bytes(reinterpret_cast<unsigned char *>(iv.data()), IV_LEN) != 1) {
        qDebug() << "Failed to generate IV:" << opensslErrString();
        return {};
    }

    if (!deriveKey(password.toUtf8(), salt, key))
        return {};

    if (!aesCrypt(data, key, iv, cipher, tag, true))
        return {};

    QByteArray result;
    const QByteArray hintBytes = hint.toUtf8();
    const quint32 hintLen = static_cast<quint32>(hintBytes.size());

    result.reserve(magicString.size() + sizeof(hintLen) + hintBytes.size() + SALT_LEN + IV_LEN + TAG_LEN + cipher.size());
    result += magicString;
    result.append(reinterpret_cast<const char *>(&hintLen), sizeof(hintLen));

    result += hintBytes;
    result += salt;
    result += iv;
    result += tag;
    result += cipher;

    return result;
}

QByteArray SystemController::decryptData(const QByteArray &content, const QString &password)
{
    if (!content.startsWith(magicString)) {
        qDebug() << "Invalid file format (magic missing)";
        return {};
    }

    qsizetype pos = magicString.size();
    if (content.size() - pos < static_cast<qsizetype>(sizeof(quint32))) {
        qDebug() << "Corrupted data (no hint length)";
        return {};
    }

    quint32 hintLen = 0;
    memcpy(&hintLen, content.constData() + pos, sizeof(hintLen));
    pos += sizeof(hintLen);

    const qsizetype hintSize = static_cast<qsizetype>(hintLen);
    if (hintSize > content.size() - pos) {
        qDebug() << "Corrupted data (hint truncated)";
        return {};
    }

    pos += hintSize;

    constexpr qsizetype encryptionMetadataSize = SALT_LEN + IV_LEN + TAG_LEN;
    if (encryptionMetadataSize > content.size() - pos) {
        qDebug() << "Corrupted data (encryption metadata truncated)";
        return {};
    }

    const QByteArray salt = content.mid(pos, SALT_LEN);
    pos += SALT_LEN;

    const QByteArray iv = content.mid(pos, IV_LEN);
    pos += IV_LEN;

    const QByteArray tag = content.mid(pos, TAG_LEN);
    pos += TAG_LEN;

    const QByteArray cipher = content.mid(pos);

    QByteArray key;
    if (!deriveKey(password.toUtf8(), salt, key)) {
        qDebug() << "Key derivation failed";
        return { };
    }

    QByteArray plain;
    QByteArray tagCopy = tag;

    if (!aesCrypt(cipher, key, iv, plain, tagCopy, false)) {
        qDebug() << "Decryption failed (wrong password or corrupted data)";
        return {};
    }

    return plain;
}

QByteArray SystemController::getDecryptedData(const QString &filePath, const QString &password)
{
    QFile f(filePath);

    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file:" << f.errorString();
        return {};
    }
    const QByteArray encryptedData = f.readAll();

    f.close();

    return decryptData(encryptedData, password);
}

bool SystemController::isFileEncrypted(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file for read: %1", f.errorString();
        return false;
    }
    QByteArray data = f.readAll();
    f.close();

    if (!data.startsWith(magicString)) {
        qDebug() << "File is not recognized as encrypted (magic missing)";
        return false;
    }

    return true;
}

bool SystemController::isPasswordValid(const QString &filePath, const QString &password)
{
    QFile f(filePath);

    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << f.errorString();
        return false;
    }

    const QByteArray encryptedData = f.readAll();
    f.close();

    if (!encryptedData.startsWith(magicString))
        return false;

    const QByteArray decrypted = decryptData(encryptedData, password);

    return !decrypted.isNull();
}

QString SystemController::readHint(const QString &filePath)
{
    QFile f(filePath);

    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file:" << f.errorString();
        return {};
    }

    const QByteArray data = f.readAll();
    f.close();

    const qsizetype pos = magicString.size();

    if (data.size() < pos + static_cast<qsizetype>(sizeof(quint32))) {
        qDebug() << "Corrupted encrypted file (no hint length)";
        return {};
    }

    quint32 hintLen = 0;
    memcpy(&hintLen, data.constData() + pos, sizeof(hintLen));

    const qsizetype hintStart = pos + static_cast<qsizetype>(sizeof(quint32));
    const qsizetype hintSize = static_cast<qsizetype>(hintLen);

    if (hintSize > data.size() - hintStart) {
        qDebug() << "Corrupted encrypted file (hint truncated)";
        return {};
    }

    return QString::fromUtf8(data.constData() + hintStart, hintSize);
}

QString SystemController::getFileName(const QString &acceptLabel, const QString &nameFilter,
                                      const QString &selectedFile, const bool isSaveMode, const QString &defaultSuffix)
{
    QString fileName;
#ifdef Q_OS_ANDROID
    Q_ASSERT(!isSaveMode);
    return AndroidController::instance()->openFile(nameFilter);
#endif

#ifdef Q_OS_IOS

    fileName = IosController::Instance()->openFile();
    if (fileName.isEmpty()) {
        return fileName;
    }
    
    CFURLRef url = CFURLCreateWithFileSystemPath(
            kCFAllocatorDefault,
            CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(fileName.unicode()), fileName.length()),
            kCFURLPOSIXPathStyle, 0);

    if (!CFURLStartAccessingSecurityScopedResource(url)) {
        qDebug() << "Could not access path " << QUrl::fromLocalFile(fileName).toString();
    }

    return fileName;
#endif

    QObject *mainFileDialog = m_qmlRoot->findChild<QObject>("mainFileDialog").parent();
    if (!mainFileDialog) {
        return "";
    }

    mainFileDialog->setProperty("acceptLabel", QVariant::fromValue(acceptLabel));
    mainFileDialog->setProperty("nameFilters", QVariant::fromValue(QStringList(nameFilter)));
    mainFileDialog->setProperty("defaultSuffix", QVariant::fromValue(defaultSuffix));
    mainFileDialog->setProperty("isSaveMode", QVariant::fromValue(isSaveMode));
    if (!selectedFile.isEmpty()) {
        mainFileDialog->setProperty("selectedFile", QVariant::fromValue(QUrl(selectedFile)));
    }
    QMetaObject::invokeMethod(mainFileDialog, "open");

    bool isFileDialogAccepted = false;
    QEventLoop wait;
    QObject::connect(this, &SystemController::fileDialogClosed, [&wait, &isFileDialogAccepted](const bool isAccepted) {
        isFileDialogAccepted = isAccepted;
        wait.quit();
    });
    wait.exec();
    QObject::disconnect(this, &SystemController::fileDialogClosed, nullptr, nullptr);

    if (!isFileDialogAccepted) {
        return "";
    }

    fileName = mainFileDialog->property("selectedFile").toString();
    return QUrl(fileName).toLocalFile();
}

void SystemController::setQmlRoot(QObject *qmlRoot)
{
    m_qmlRoot = qmlRoot;
}

bool SystemController::isAuthenticated()
{
#ifdef Q_OS_ANDROID
    return AndroidController::instance()->requestAuthentication();
#else
    return true;
#endif
}

void SystemController::sendTouch(float x, float y)
{
#ifdef Q_OS_ANDROID
    AndroidController::instance()->sendTouch(x, y);
#endif
}
