#include "debug.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QCoreApplication>
#include <QDateTime>

#define LOGS_DIR "logs"
#define CLIENT_LOG_SUFFIX "amneziavpn.log"
#define MAX_LOG_FILES 5
#define FORMAT_STRING "yyyy-MM-dd--hh-mm-ss"

QFile Debug::m_clientLog;
QTextStream Debug::m_clientLogTextStream;
QString Debug::m_clientLogName;

void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Skip Qt warnings
    if (msg.contains("known incorrect sRGB profile")) return;
    if (msg.contains("libpng warning")) return;
    if (msg.contains("Unknown property ffont")) return;

    Debug::m_clientLogTextStream << qFormatLogMessage(type, context, msg) << endl << flush;
}

bool Debug::init()
{
    QString path = qApp->applicationDirPath();
    QDir appDir(path);

    // init function is called before exec application, so data location folder may not exist
    if (!appDir.exists())
    {
        qWarning() << "Debug: init: log directory doesn't exist or mkpath command error:" << path;
        return false;
    }

    if (!appDir.exists(LOGS_DIR) && !appDir.mkdir(LOGS_DIR))
    {
        qWarning() << "Debug: init: log directory doesn't exist or mkdir command error:" << path << LOGS_DIR;
        return false;
    }

    if (!appDir.cd(LOGS_DIR))
    {
        qWarning() << "Debug: init: cd command error:" << path << LOGS_DIR;
        return false;
    }

    //delete older log files
    auto clientLogsCount = 0;
    QFileInfoList logDirList = appDir.entryInfoList(
                QDir::Files | QDir::NoDotAndDotDot,
                QDir::Time);
    for (auto fileInfo : logDirList)
    {
        if ((fileInfo.completeSuffix() == CLIENT_LOG_SUFFIX &&
                ++clientLogsCount > MAX_LOG_FILES))
        {
            appDir.remove(fileInfo.filePath());
        }
    }

    //prepare log file names
    auto currentDateTime = QDateTime::currentDateTime().toString(FORMAT_STRING);

    m_clientLogName = QString("%1.%2").arg(currentDateTime).arg(CLIENT_LOG_SUFFIX);
    return init(appDir);
}

bool Debug::init(QDir& appDir)
{
    Q_UNUSED(appDir)
    qSetMessagePattern("[%{time}|%{type}] %{message}");

#ifndef QT_DEBUG
    m_clientLog.setFileName(appDir.filePath(m_clientLogName));
    if (!m_clientLog.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Debug::init - failed to open m_clientLog file:" << m_clientLogName;
        return false;
    }
    m_clientLog.setTextModeEnabled(true);
    m_clientLogTextStream.setDevice(&m_clientLog);
    qInstallMessageHandler(debugMessageHandler);
#else
#ifdef DEBUG_OUTPUT_TWO_DIRECTIONAL
    m_clientLog.setFileName(appDir.filePath(m_clientLogName));
    if (!m_clientLog.open(QIODevice::WriteOnly | QIODevice::Append))
        return false;
    m_clientLog.setTextModeEnabled(true);
    m_clientLogTextStream.setDevice(&m_clientLog);
    defaultMessageHandler = qInstallMessageHandler(debugMessageHandler);
#endif
#endif

#ifndef Q_OS_WIN
    if (!fixOvpnLogPermissions())
        qWarning() << "Debug: permissions for ovpn.log were not fixed";
#endif
    return true;
}

QString Debug::getPathToClientLog()
{
    QString path = qApp->applicationDirPath();
    QDir appDir(path);
    if (!appDir.exists(LOGS_DIR) || !appDir.cd(LOGS_DIR))
    {
        qWarning() << "Debug: log directory doesn't exist or cd command error:" << path;
        return "";
    }

    return appDir.filePath(m_clientLogName);
}

QString Debug::getPathToLogsDir()
{
    QString path = qApp->applicationDirPath();
    QDir appDir(path);
    if (!appDir.exists(LOGS_DIR) || !appDir.cd(LOGS_DIR))
    {
        qWarning() << "Debug: log directory doesn't exist or cd command error" << path;
        return "";
    }
    return appDir.absolutePath();
}


