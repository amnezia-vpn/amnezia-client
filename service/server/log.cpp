#include <QDir>
#include <QStandardPaths>

#include <iostream>

#include "log.h"
#include "defines.h"
#include "utils.h"

QFile Log::m_file;
QTextStream Log::m_textStream;
QString Log::m_logFileName;

void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (msg.simplified().isEmpty()) {
        return;
    }

    Log::m_textStream << qFormatLogMessage(type, context, msg) << endl << flush;

    std::cout << qFormatLogMessage(type, context, msg).toStdString() << std::endl << std::flush;
}

bool Log::initialize()
{
    QString path = Utils::systemLogPath();
    QDir appDir(path);
    if (!appDir.mkpath(path)) {
        return false;
    }

    m_logFileName = QString("%1.log").arg(SERVICE_NAME);

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

QString Log::serviceLogFileNamePath()
{
    return m_file.fileName();
}

