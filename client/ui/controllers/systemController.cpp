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

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
    #include <CoreFoundation/CoreFoundation.h>
#endif

namespace
{
    const char *saveFileResultToString(SystemController::SaveFileResult result)
    {
        switch (result) {
        case SystemController::SaveFileResult::Saved: return "Saved";
        case SystemController::SaveFileResult::Cancelled: return "Cancelled";
        case SystemController::SaveFileResult::Unknown: return "Unknown";
        case SystemController::SaveFileResult::Failed: return "Failed";
        }
        return "?";
    }
}

SystemController *SystemController::s_instance = nullptr;

SystemController::SystemController(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

SystemController::~SystemController()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void SystemController::notifySaveCancelled()
{
    if (s_instance) {
        qDebug() << "[saveFile] SystemController::notifySaveCancelled: emitting saveCancelledByUser";
        emit s_instance->saveCancelledByUser();
    } else {
        qWarning() << "[saveFile] SystemController::notifySaveCancelled: no SystemController instance";
    }
}

SystemController::SaveFileResult SystemController::saveFileEx(const QString &fileName, const QString &data)
{
#if defined Q_OS_ANDROID
    qDebug() << "SystemController::saveFile: android, name:" << fileName << "size:" << data.size();
    const SaveFileResult result = AndroidController::instance()->saveFile(fileName, data);
    qDebug() << "SystemController::saveFile: android result:" << saveFileResultToString(result);
    return result;
#else
    return saveFileEx(fileName, data.toUtf8());
#endif
}

SystemController::SaveFileResult SystemController::saveFileEx(const QString &fileName, const QByteArray &data)
{
#if defined Q_OS_ANDROID
    qDebug() << "SystemController::saveFile: android, name:" << fileName << "size:" << data.size();
    const SaveFileResult androidResult = AndroidController::instance()->saveFile(fileName, QString::fromUtf8(data));
    qDebug() << "SystemController::saveFile: android result:" << saveFileResultToString(androidResult);
    return androidResult;
#endif

#ifdef Q_OS_IOS
    const QString filePath = QDir::tempPath() + "/" + fileName;
    QFile file(filePath);
    qDebug() << "SystemController::saveFile: ios, staging" << data.size() << "bytes in" << filePath;
#else
    QFile file(fileName);
    qDebug() << "SystemController::saveFile: desktop, writing" << data.size() << "bytes to" << fileName;
#endif

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "SystemController::saveFile: cannot open" << fileName;
        return SaveFileResult::Failed;
    }
    if (file.write(data) != data.size()) {
        qWarning() << "SystemController::saveFile: write failed" << fileName;
        file.close();
        return SaveFileResult::Failed;
    }
    file.close();

#ifdef Q_OS_IOS
    QStringList filesToSend;
    filesToSend.append(filePath);

    IosController::ShareResult shareResult;
    qDebug() << "SystemController::saveFile: ios, presenting the share sheet for" << fileName;
    IosController::Instance()->shareText(filesToSend, &shareResult);
    qDebug() << "SystemController::saveFile: ios share sheet closed, completed:" << shareResult.completed
             << "activityType:" << (shareResult.activityType.isEmpty() ? QStringLiteral("<none>") : shareResult.activityType)
             << "error:" << (shareResult.errorString.isEmpty() ? QStringLiteral("<none>") : shareResult.errorString);

    // the temporary copy carries the config (and its private key), do not leave it behind
    if (!QFile::remove(filePath)) {
        qWarning() << "SystemController::saveFile: could not remove the temporary copy" << filePath;
    }

    if (shareResult.completed) {
        return SaveFileResult::Saved;
    }
    // an empty activityType means no activity was ever started, i.e. the user closed the share sheet
    if (shareResult.activityType.isEmpty()) {
        if (!shareResult.errorString.isEmpty()) {
            qWarning() << "SystemController::saveFile: cannot present the share sheet:" << shareResult.errorString;
            return SaveFileResult::Failed;
        }
        qDebug() << "SystemController::saveFile: ios, the user dismissed the share sheet without picking a destination";
        return SaveFileResult::Cancelled;
    }
    // a destination was picked but reported no success; some third-party share extensions
    // report this even after a successful send, so do not claim either outcome
    qWarning() << "SystemController::saveFile: share did not report success for" << shareResult.activityType
               << shareResult.errorString;
    return SaveFileResult::Unknown;
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
    return SaveFileResult::Saved;
#endif
}

bool SystemController::saveFile(const QString &fileName, const QString &data)
{
    qDebug() << "[saveFile] SystemController::saveFile: start, name:" << fileName << "size:" << data.size();
    const SaveFileResult result = saveFileEx(fileName, data);
    const bool saved = result == SaveFileResult::Saved;
    qDebug() << "[saveFile] SystemController::saveFile:" << fileName << "->" << saveFileResultToString(result)
             << "returning" << saved;
    if (result == SaveFileResult::Cancelled) {
        notifySaveCancelled();
    }
    return saved;
}

bool SystemController::saveFile(const QString &fileName, const QByteArray &data)
{
    qDebug() << "[saveFile] SystemController::saveFile: start, name:" << fileName << "size:" << data.size();
    const SaveFileResult result = saveFileEx(fileName, data);
    const bool saved = result == SaveFileResult::Saved;
    qDebug() << "[saveFile] SystemController::saveFile:" << fileName << "->" << saveFileResultToString(result)
             << "returning" << saved;
    if (result == SaveFileResult::Cancelled) {
        notifySaveCancelled();
    }
    return saved;
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
        qDebug() << "SystemController::getFileName: dialog cancelled by the user, isSaveMode:" << isSaveMode;
        if (isSaveMode) {
            notifySaveCancelled();
        }
        return "";
    }

    fileName = mainFileDialog->property("selectedFile").toString();
    qDebug() << "SystemController::getFileName: picked" << fileName << "isSaveMode:" << isSaveMode;
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
