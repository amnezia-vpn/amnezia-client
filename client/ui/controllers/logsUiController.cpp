#include "logsUiController.h"

#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QLocale>
#include <QtConcurrent>

#include "core/controllers/logsController.h"
#include "systemController.h"
#include "version.h"

#ifdef Q_OS_ANDROID
    #include <QDir>
    #include <QStandardPaths>

    #include "platforms/android/android_controller.h"
#endif

namespace
{
    constexpr qsizetype kLiveTailTextLimit = 2 * 1024 * 1024;

    LogsController::Stream streamFromString(const QString &stream)
    {
        return stream == QLatin1String("tunnel") ? LogsController::Stream::Tunnel : LogsController::Stream::App;
    }

    QString newestAppLogFile()
    {
        const QStringList files = LogsController::files(LogsController::Stream::App);
        return files.isEmpty() ? QString() : files.last();
    }

#ifdef Q_OS_ANDROID
    bool shareFile(const QString &fileName, const QByteArray &data, const QString &mimeType)
    {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/share"));
        dir.removeRecursively();
        if (!dir.mkpath(QStringLiteral("."))) {
            qWarning() << "LogsUiController: cannot create" << dir.path();
            return false;
        }

        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) {
            qWarning() << "LogsUiController: cannot write" << file.fileName();
            return false;
        }
        file.close();

        AndroidController::instance()->shareFile(file.fileName(), mimeType);
        return true;
    }
#endif
}

LogsUiController::LogsUiController(QObject *parent) : QObject(parent)
{
    m_liveTailTimer.setInterval(1000);
    connect(&m_liveTailTimer, &QTimer::timeout, this, &LogsUiController::onLiveTailTimeout);
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &LogsUiController::onApplicationStateChanged);
}

QString LogsUiController::appLogSize() const
{
    return QLocale().formattedDataSize(LogsController::size(LogsController::Stream::App));
}

QString LogsUiController::tunnelLogSize() const
{
    return QLocale().formattedDataSize(LogsController::size(LogsController::Stream::Tunnel));
}

QString LogsUiController::viewerStream() const
{
    return m_viewerStream;
}

void LogsUiController::setViewerStream(const QString &stream)
{
    if (m_viewerStream == stream) {
        return;
    }
    m_viewerStream = stream;
    emit viewerStreamChanged();
}

bool LogsUiController::busy() const
{
    return m_busy;
}

bool LogsUiController::canShare() const
{
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

void LogsUiController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void LogsUiController::refreshSizes()
{
    emit sizesChanged();
}

void LogsUiController::load(const QString &stream)
{
    const quint64 loadId = ++m_loadId;
    const LogsController::Stream logsStream = streamFromString(stream);

    auto *watcher = new QFutureWatcher<LoadResult>(this);
    connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this, watcher, stream, logsStream, loadId]() {
        const LoadResult result = watcher->result();
        watcher->deleteLater();

        if (loadId != m_loadId) {
            return;
        }

        m_lastText = result.text;
        if (logsStream == LogsController::Stream::App) {
            m_tailFile = result.tailFile;
            m_tailSize = result.tailSize;
        }
        emit streamLoaded(stream, result.text);
    });

    watcher->setFuture(QtConcurrent::run([logsStream]() {
        LoadResult result;
        if (logsStream == LogsController::Stream::App) {
            result.tailFile = newestAppLogFile();
        }
        result.text = LogsController::readTail(logsStream, &result.tailSize);
        return result;
    }));
}

void LogsUiController::startLiveTail()
{
    m_liveTailEnabled = true;
    m_tailFile = newestAppLogFile();
    m_tailSize = LogsController::fileSize(m_tailFile);
    m_liveTailTimer.start();
}

void LogsUiController::stopLiveTail()
{
    m_liveTailEnabled = false;
    m_liveTailTimer.stop();
}

void LogsUiController::copyLoaded()
{
    QGuiApplication::clipboard()->setText(m_lastText);
}

void LogsUiController::exportStream(const QString &stream, const QString &fileName)
{
    const LogsController::Stream logsStream = streamFromString(stream);
    runExport([logsStream]() { return LogsController::readAll(logsStream); },
              [fileName](const QByteArray &data) { return SystemController::saveFile(fileName, data); });
}

void LogsUiController::saveAll(const QString &fileName)
{
    runExport([]() { return LogsController::zipAll(); },
              [fileName](const QByteArray &data) {
                  return SystemController::saveFile(fileName, data, QStringLiteral("application/zip"));
              });
}

void LogsUiController::shareAll()
{
#ifdef Q_OS_ANDROID
    const QString fileName = defaultFileName(QStringLiteral("all"));
    runExport([]() { return LogsController::zipAll(); },
              [fileName](const QByteArray &data) {
                  return shareFile(fileName, data, QStringLiteral("application/zip"));
              });
#endif
}

void LogsUiController::shareStream(const QString &stream)
{
#ifdef Q_OS_ANDROID
    const LogsController::Stream logsStream = streamFromString(stream);
    const QString fileName = defaultFileName(stream);
    runExport([logsStream]() { return LogsController::readAll(logsStream); },
              [fileName](const QByteArray &data) { return shareFile(fileName, data, QStringLiteral("text/plain")); });
#else
    Q_UNUSED(stream);
#endif
}

void LogsUiController::runExport(const std::function<QByteArray()> &build,
                                 const std::function<bool(const QByteArray &)> &deliver)
{
    if (m_busy) {
        return;
    }
    setBusy(true);

    auto *watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [this, watcher, deliver]() {
        const QByteArray data = watcher->result();
        watcher->deleteLater();

        const bool success = deliver(data);
        if (!success) {
            qInfo() << "LogsUiController: save or share was cancelled or failed";
        }
        setBusy(false);
        emit exportFinished(success);
    });

    watcher->setFuture(QtConcurrent::run(build));
}

QString LogsUiController::defaultFileName(const QString &kind) const
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    if (kind == QLatin1String("all")) {
        return QStringLiteral("%1-logs-%2.zip").arg(APPLICATION_NAME, timestamp);
    }

    const QString stream = kind == QLatin1String("tunnel") ? QStringLiteral("tunnel") : QStringLiteral("app");
    return QStringLiteral("%1-%2-%3.log").arg(APPLICATION_NAME, stream, timestamp);
}

void LogsUiController::onLiveTailTimeout()
{
    const QString file = newestAppLogFile();
    QFile logFile(file);
    const qint64 size = logFile.open(QIODevice::ReadOnly) ? logFile.size() : 0;

    if (file != m_tailFile || size < m_tailSize) {
        m_tailFile = file;
        m_tailSize = size;
        emit reloadRequired();
        return;
    }

    if (size == m_tailSize) {
        return;
    }

    if (!logFile.seek(m_tailSize)) {
        return;
    }

    QByteArray delta = logFile.read(size - m_tailSize);
    const qsizetype lastNewline = delta.lastIndexOf('\n');
    if (lastNewline < 0) {
        return;
    }
    delta.truncate(lastNewline + 1);
    m_tailSize += delta.size();

    const QString text = QString::fromUtf8(delta);
    m_lastText += text;
    if (m_lastText.size() > kLiveTailTextLimit) {
        emit reloadRequired();
        return;
    }
    emit appended(text);
}

void LogsUiController::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (!m_liveTailEnabled) {
        return;
    }

    if (state == Qt::ApplicationHidden || state == Qt::ApplicationSuspended) {
        m_liveTailTimer.stop();
        return;
    }

    if (!m_liveTailTimer.isActive()) {
        m_liveTailTimer.start();
        emit reloadRequired();
    }
}
