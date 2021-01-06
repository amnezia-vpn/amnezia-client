#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include <iostream>

#include "debug.h"
#include "defines.h"

QFile Debug::m_file;
QTextStream Debug::m_textStream;
QString Debug::m_logFileName;

void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (msg.simplified().isEmpty()) {
        return;
    }

    // Skip annoying messages from Qt
    if (msg.startsWith("Unknown property") || msg.startsWith("Could not create pixmap") || msg.startsWith("Populating font")) {
        return;
    }

    Debug::m_textStream << qFormatLogMessage(type, context, msg) << endl << flush;

    std::cout << qFormatLogMessage(type, context, msg).toStdString() << std::endl << std::flush;
}

bool Debug::init()
{
    QString path = userLogsDir();
    QDir appDir(path);
    if (!appDir.mkpath(path)) {
        return false;
    }

    m_logFileName = QString("%1.log").arg(APPLICATION_NAME);

    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} %{type} %{message}");

    m_file.setFileName(appDir.filePath(m_logFileName));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Cannot open log file:" << m_logFileName;
        return false;
    }
    m_file.setTextModeEnabled(true);
    m_textStream.setDevice(&m_file);
    qInstallMessageHandler(debugMessageHandler);

    return true;
}

QString Debug::userLogsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/log";
}

bool Debug::openLogsFolder()
{
    QString path = userLogsDir();
#ifdef Q_OS_WIN
    path = "file:///" + path;
#endif
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        qWarning() << "Can't open url:" << path;
        return false;
    }
    return true;
}

QString Debug::appLogFileNamePath()
{
    return m_file.fileName();
}
