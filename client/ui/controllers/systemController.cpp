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
    constexpr int IV_LEN = 16;
    constexpr int KEY_LEN = 32;
    constexpr int PBKDF2_ITER = 100000;

    const QByteArray magicString { "EncData" };
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
    const unsigned char *pw = reinterpret_cast<const unsigned char *>(password.constData());
    const unsigned char *s = reinterpret_cast<const unsigned char *>(salt.constData());
    int ok = PKCS5_PBKDF2_HMAC(reinterpret_cast<const char *>(pw), password.size(), s, salt.size(), PBKDF2_ITER,
                               EVP_sha256(), KEY_LEN, reinterpret_cast<unsigned char *>(outKey.data()));
    if (!ok) {
        qDebug() << opensslErrString();
    }
    return ok == 1;
}

static bool aesCrypt(const QByteArray &in, const QByteArray &key, const QByteArray &iv, QByteArray &out, bool encrypt)
{
    std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX *)> ctx { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };

    if (!ctx) {
        qDebug() << "EVP_CIPHER_CTX_new failed";
        return false;
    }

    const EVP_CIPHER *cipher = EVP_aes_256_cbc();

    if (1 != EVP_CipherInit_ex(ctx.get(), cipher, nullptr, reinterpret_cast<const unsigned char *>(key.constData()),
                               reinterpret_cast<const unsigned char *>(iv.constData()), encrypt ? 1 : 0)) {
        qDebug() << opensslErrString();
        return false;
    }

    out.clear();
    out.resize(in.size() + EVP_CIPHER_block_size(cipher));

    int outlen1 = 0;
    if (1 != EVP_CipherUpdate(ctx.get(), reinterpret_cast<unsigned char *>(out.data()), &outlen1,
                              reinterpret_cast<const unsigned char *>(in.constData()), in.size())) {
        qDebug() << opensslErrString();
        return false;
    }

    int outlen2 = 0;
    if (1 != EVP_CipherFinal_ex(ctx.get(), reinterpret_cast<unsigned char *>(out.data()) + outlen1, &outlen2)) {
        qDebug() << opensslErrString();
        return false;
    }

    out.resize(outlen1 + outlen2);
    return true;
}

bool SystemController::encryptFile(const QString &filePath, const QString &password, const QString &hint)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << QStringLiteral("Cannot open file for read: %1").arg(f.errorString());
        return false;
    }
    QByteArray content = f.readAll();
    f.close();

    if (content.startsWith(magicString)) {
        qDebug() << QStringLiteral("File already encrypted (magic found)");
        return false;
    }

    QByteArray qba_hint = hint.toUtf8();
    quint32 qba_hint_len = static_cast<quint32>(qba_hint.size());
    QByteArray salt(SALT_LEN, 0);
    QByteArray iv(IV_LEN, 0);
    QByteArray key;
    QByteArray cipher;
    QByteArray out;

    if (1 != RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), SALT_LEN)
        || 1 != RAND_bytes(reinterpret_cast<unsigned char *>(iv.data()), IV_LEN)) {
        qDebug() << opensslErrString();
        return false;
    }

    if (!deriveKey(password.toUtf8(), salt, key))
        return false;

    if (!aesCrypt(content, key, iv, cipher, true))
        return false;

    out.reserve(magicString.size() + SALT_LEN + IV_LEN + cipher.size());
    out += magicString;
    out.append(reinterpret_cast<const char *>(&qba_hint_len), sizeof(qba_hint_len));
    out += hint.toUtf8();
    out += salt;
    out += iv;
    out += cipher;

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << QStringLiteral("Cannot open file for write: %1").arg(f.errorString());
        return false;
    }
    qint64 written = f.write(out);
    f.close();
    if (written != out.size()) {
        qDebug() << QStringLiteral("Write failed or incomplete");
        return false;
    }
    return true;
}

QByteArray SystemController::getDecryptedData(const QString &filePath, const QString &password)
{
    QByteArray encData;
    readFile(filePath, encData);

    int pos = magicString.size();

    quint32 hintLen = 0;
    memcpy(&hintLen, encData.constData() + pos, sizeof(quint32));
    pos += sizeof(quint32);
    pos += hintLen;

    QByteArray salt = encData.mid(pos, 16);
    pos += 16;
    QByteArray iv = encData.mid(pos, 16);
    pos += 16;
    QByteArray cipher = encData.mid(pos);

    QByteArray key;
    deriveKey(password.toUtf8(), salt, key);

    QByteArray data;
    !aesCrypt(cipher, key, iv, data, false);

    return data;
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

    QByteArray content = f.readAll();
    f.close();

    int pos = magicString.size();
    quint32 hintLen = 0;
    memcpy(&hintLen, content.constData() + pos, sizeof(quint32));
    pos += sizeof(quint32);
    pos += hintLen;

    QByteArray salt = content.mid(pos, 16);
    pos += 16;
    QByteArray iv = content.mid(pos, 16);
    pos += 16;
    QByteArray cipher = content.mid(pos);

    QByteArray key;
    if (!deriveKey(password.toUtf8(), salt, key))
        return false;

    QByteArray plain;
    bool ok = aesCrypt(cipher, key, iv, plain, false);

    if (!ok) {
        qDebug() << "Wrong password";
        return false;
    }

    return true;
}

QString SystemController::readHint(const QString &filePath)
{
    if (filePath.isEmpty())
        return "";

    QByteArray data;
    readFile(filePath, data);

    int pos = magicString.size();

    if (data.size() < pos + static_cast<int>(sizeof(quint32))) {
        qDebug() << "Corrupted file (no hint length)";
        return {};
    }

    quint32 hintLen = 0;
    memcpy(&hintLen, data.constData() + pos, sizeof(quint32));
    pos += sizeof(quint32);

    if (data.size() < pos + static_cast<int>(hintLen)) {
        qDebug() << "Corrupted file (hint truncated)";
        return {};
    }

    return QString::fromUtf8(data.constData() + pos, hintLen);
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
