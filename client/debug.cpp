#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include <iostream>

#include <core/ipcclient.h>

#include "debug.h"
#include "defines.h"
#include "utils.h"

QFile Debug::m_file;
QTextStream Debug::m_textStream;
QString Debug::m_logFileName = QString("%1.log").arg(APPLICATION_NAME);

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
    Debug::appendAllLog(qFormatLogMessage(type, context, msg));

    std::cout << qFormatLogMessage(type, context, msg).toStdString() << std::endl << std::flush;
}

Debug &Debug::Instance()
{
    static Debug s;
    return s;
}

void Debug::appendSshLog(const QString &log)
{
    QString dt = QDateTime::currentDateTime().toString();
    Instance().m_sshLog.append(dt + ": " + log + "\n");
    emit Instance().sshLogChanged(Instance().sshLog());
}

void Debug::appendAllLog(const QString &log)
{
    Instance().m_allLog.append(log + "\n");
    emit Instance().allLogChanged(Instance().allLog());
}

bool Debug::init()
{
    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} %{type} %{message}");

    QString path = userLogsDir();
    QDir appDir(path);
    if (!appDir.mkpath(path)) {
        return false;
    }

    m_file.setFileName(appDir.filePath(m_logFileName));
    if (!m_file.open(QIODevice::Append)) {
        qWarning() << "Cannot open log file:" << m_logFileName;
        return false;
    }
    m_file.setTextModeEnabled(true);
    m_textStream.setDevice(&m_file);

#ifndef QT_DEBUG
    qInstallMessageHandler(debugMessageHandler);
#endif

    return true;
}

QString Debug::userLogsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/log";
}

QString Debug::userLogsFilePath()
{
    return userLogsDir() + QDir::separator() + m_logFileName;
}

QString Debug::getLogFile()
{
    m_file.flush();
    QFile file(userLogsFilePath());

    file.open(QIODevice::ReadOnly);
    return file.readAll();
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

bool Debug::openServiceLogsFolder()
{
    QString path = Utils::systemLogPath();
    path = "file:///" + path;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    return true;
}

QString Debug::appLogFileNamePath()
{
    return m_file.fileName();
}

void Debug::clearLogs()
{
    bool isLogActive = m_file.isOpen();
    m_file.close();

    QFile file(userLogsFilePath());

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.resize(0);
    file.close();

    if (isLogActive) {
        init();
    }
}

void Debug::clearServiceLogs()
{
    IpcClient *m_IpcClient = new IpcClient;

    if (!m_IpcClient->isSocketConnected()) {
        if (!IpcClient::init(m_IpcClient)) {
            qWarning() << "Error occured when init IPC client";
            return;
        }
    }

    if (m_IpcClient->Interface()) {
        m_IpcClient->Interface()->setLogsEnabled(false);
        m_IpcClient->Interface()->cleanUp();
    }
    else {
        qWarning() << "Error occured cleaning up service logs";
    }
}

void Debug::cleanUp()
{
    clearLogs();
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.removeRecursively();

    clearServiceLogs();
}
