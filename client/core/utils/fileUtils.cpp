#include "fileUtils.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include "platforms/android/android_controller.h"
#endif

#ifdef Q_OS_IOS
#include "platforms/ios/ios_controller.h"
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace FileUtils {

bool readFile(const QString &fileName, QByteArray &data)
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

bool readFile(const QString &fileName, QString &data)
{
    QByteArray byteArray;
    if(!readFile(fileName, byteArray)) return false;
    data = byteArray;
    return true;
}

void saveFile(const QString &fileName, const QString &data)
{
#if defined Q_OS_ANDROID
    AndroidController::instance()->saveFile(fileName, data);
    return;
#endif

#ifdef Q_OS_IOS
    QUrl fileUrl = QDir::tempPath() + "/" + fileName;
    QFile file(fileUrl.toString());
#else
    QFile file(fileName);
#endif

    file.open(QIODevice::WriteOnly);
    file.write(data.toUtf8());
    file.close();

#ifdef Q_OS_IOS
    QStringList filesToSend;
    filesToSend.append(fileUrl.toString());
    IosController::Instance()->shareText(filesToSend);
    return;
#else
    QFileInfo fi(fileName);
#ifdef Q_OS_MAC
    const auto url = "file://" + fi.absoluteDir().absolutePath();
#else
    const auto url = fi.absoluteDir().absolutePath();
#endif
    QDesktopServices::openUrl(url);
#endif
}

} // namespace FileUtils


