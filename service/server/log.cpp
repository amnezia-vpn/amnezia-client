#include <QDir>
#include <QStandardPaths>

#include <iostream>

#include "log.h"
#include "defines.h"
#include "utilities.h"

QFile Log::m_file;
QTextStream Log::m_textStream;
QString Log::m_logFileName = QString("%1.log").arg(SERVICE_NAME);

void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (msg.simplified().isEmpty()) {
        return;
    }

    Log::m_textStream << qFormatLogMessage(type, context, msg) << Qt::endl << Qt::flush;

    std::cout << qFormatLogMessage(type, context, msg).toStdString() << std::endl << std::flush;
}

bool Log::init()
{
    if (m_file.isOpen()) return true;

    QString path = Utils::systemLogPath();
    QDir appDir(path);
    if (!appDir.mkpath(path)) {
        return false;
    }

    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} %{type} %{message}");

    m_file.setFileName(appDir.filePath(m_logFileName));
    if (!m_file.open(QIODevice::Append)) {
        qWarning() << "Cannot open log file:" << m_logFileName;
        return false;
    }
    m_file.setTextModeEnabled(true);
    m_textStream.setDevice(&m_file);
    qInstallMessageHandler(debugMessageHandler);

    return true;
}

void Log::deinit()
{
    m_file.close();
    m_textStream.setDevice(nullptr);
    qInstallMessageHandler(nullptr);
}

QString Log::serviceLogFileNamePath()
{
    return m_file.fileName();
}

void Log::clearLogs()
{
    bool isLogActive = m_file.isOpen();
    m_file.close();


    QString path = Utils::systemLogPath();
    QDir appDir(path);
    QFile file;
    file.setFileName(appDir.filePath(m_logFileName));

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.resize(0);
    file.close();

    if (isLogActive) {
        init();
    }
}

void Log::cleanUp()
{
    clearLogs();
    deinit();

    QString path = Utils::systemLogPath();
    QDir appDir(path);

    {
        QFile file;
        file.setFileName(appDir.filePath(m_logFileName));
        file.remove();
    }
    {
        QFile file;
        file.setFileName(appDir.filePath("openvpn.log"));
        file.remove();
    }

#ifdef Q_OS_WINDOWS
    QDir dir(Utils::systemLogPath());
    dir.removeRecursively();
#endif
}
