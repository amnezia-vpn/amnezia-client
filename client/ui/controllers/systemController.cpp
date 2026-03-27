#include "systemController.h"

#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QTcpSocket>
#include <QElapsedTimer>

#ifdef Q_OS_ANDROID
    #include "platforms/android/android_controller.h"
#endif

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
    #include <CoreFoundation/CoreFoundation.h>
#endif

SystemController::SystemController(const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

namespace
{
    struct PingTarget
    {
        QString host;
        QList<quint16> ports;
    };

    PingTarget parsePingTarget(const QString &rawHost)
    {
        PingTarget result;
        QString host = rawHost.trimmed();
        quint16 explicitPort = 0;

        if (host.startsWith('[')) {
            const int closingBracketIndex = host.indexOf(']');
            if (closingBracketIndex > 0) {
                const QString bracketedHost = host.mid(1, closingBracketIndex - 1);
                const QString remainder = host.mid(closingBracketIndex + 1);
                host = bracketedHost;

                if (remainder.startsWith(':')) {
                    explicitPort = static_cast<quint16>(remainder.mid(1).toUShort());
                }
            }
        } else {
            const int colonCount = host.count(':');
            if (colonCount == 1) {
                const int colonIndex = host.lastIndexOf(':');
                explicitPort = static_cast<quint16>(host.mid(colonIndex + 1).toUShort());
                host = host.left(colonIndex);
            }
        }

        result.host = host.trimmed();
        if (explicitPort > 0) {
            result.ports.append(explicitPort);
        }

        for (quint16 fallbackPort : {443, 80, 22}) {
            if (!result.ports.contains(fallbackPort)) {
                result.ports.append(fallbackPort);
            }
        }

        return result;
    }
}

void SystemController::saveFile(const QString &fileName, const QString &data)
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

    // todo check if save successful
    (void)file.open(QIODevice::WriteOnly);
    file.write(data.toUtf8());
    file.close();

#ifdef Q_OS_IOS
    QStringList filesToSend;
    filesToSend.append(fileUrl.toString());
    // todo check if save successful
    IosController::Instance()->shareText(filesToSend);
    return;
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

void SystemController::measurePing(const QString &host)
{
    if (host.isEmpty()) {
        emit pingMeasured(-1);
        return;
    }

    const auto target = parsePingTarget(host);
    if (target.host.isEmpty()) {
        emit pingMeasured(-1);
        return;
    }

    const quint64 requestId = ++m_pingRequestId;
    auto ports = std::make_shared<QList<quint16>>(target.ports);
    auto elapsed = std::make_shared<QElapsedTimer>();
    elapsed->start();

    auto tryNextPort = std::make_shared<std::function<void()>>();
    *tryNextPort = [this, requestId, target, ports, elapsed, tryNextPort]() {
        if (requestId != m_pingRequestId) {
            return;
        }

        if (ports->isEmpty()) {
            emit pingMeasured(-1);
            return;
        }

        const quint16 port = ports->takeFirst();
        auto *socket = new QTcpSocket(this);
        auto *timeout = new QTimer(socket);
        timeout->setSingleShot(true);

        auto finished = std::make_shared<bool>(false);
        auto cleanup = [socket, timeout, finished]() {
            if (*finished) {
                return false;
            }
            *finished = true;
            timeout->stop();
            socket->abort();
            socket->deleteLater();
            return true;
        };

        connect(socket, &QTcpSocket::connected, this, [this, requestId, elapsed, cleanup]() {
            if (!cleanup() || requestId != m_pingRequestId) {
                return;
            }

            emit pingMeasured(static_cast<int>(elapsed->elapsed()));
        });

        void(QTcpSocket::*errorSignal)(QAbstractSocket::SocketError) = &QTcpSocket::errorOccurred;
        connect(socket, errorSignal, this, [this, requestId, cleanup, tryNextPort](QAbstractSocket::SocketError) {
            if (!cleanup() || requestId != m_pingRequestId) {
                return;
            }

            (*tryNextPort)();
        });

        connect(timeout, &QTimer::timeout, this, [this, requestId, cleanup, tryNextPort]() {
            if (!cleanup() || requestId != m_pingRequestId) {
                return;
            }

            (*tryNextPort)();
        });

        socket->connectToHost(target.host, port);
        timeout->start(1500);
    };

    (*tryNextPort)();
}
