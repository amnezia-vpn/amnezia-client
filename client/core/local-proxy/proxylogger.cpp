#include "proxylogger.h"
#include <QDir>
#include <QTextStream>

ProxyLogger::ProxyLogger() : m_maxFileSize(0), m_currentLevel(LogLevel::Info)
{
}

ProxyLogger::~ProxyLogger()
{
}

ProxyLogger& ProxyLogger::getInstance()
{
    static ProxyLogger instance;
    return instance;
}

void ProxyLogger::init(const QString& logPath, qint64 maxFileSize)
{
    QMutexLocker locker(&m_mutex);
    m_logPath = logPath;
    m_maxFileSize = maxFileSize;
    
    // Create logs directory if it doesn't exist
    QDir dir = QFileInfo(m_logPath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void ProxyLogger::setLogLevel(LogLevel level)
{
    m_currentLevel = level;
}

void ProxyLogger::log(LogLevel level, const QString& message)
{
    logInternal(level, message);
}

void ProxyLogger::debug(const QString& message)
{
    logInternal(LogLevel::Debug, message);
}

void ProxyLogger::info(const QString& message)
{
    logInternal(LogLevel::Info, message);
}

void ProxyLogger::warning(const QString& message)
{
    logInternal(LogLevel::Warning, message);
}

void ProxyLogger::error(const QString& message)
{
    logInternal(LogLevel::Error, message);
}

void ProxyLogger::logInternal(LogLevel level, const QString& message)
{
    if (m_logPath.isEmpty()) {
        return;
    }
    
    if (level < m_currentLevel) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    
    checkRotation();

    QFile file(m_logPath);
    if (!openLogFile(file)) {
        return;
    }

    QTextStream stream(&file);
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    stream << QString("[%1] [%2] %3\n").arg(timestamp, levelToString(level), message);
    stream.flush();
    file.close();
}

QString ProxyLogger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        default:               return "UNKNOWN";
    }
}

qint64 ProxyLogger::getCurrentFileSize() const
{
    QFile file(m_logPath);
    if (file.exists()) {
        return file.size();
    }
    return -1;
}

void ProxyLogger::checkRotation()
{
    if (m_maxFileSize > 0 && getCurrentFileSize() >= m_maxFileSize) {
        // Delete the oldest file
        QFile::remove(QString("%1.%2").arg(m_logPath).arg(MAX_BACKUP_FILES));

        // Shift existing files
        for (int i = MAX_BACKUP_FILES - 1; i >= 1; --i) {
            QString oldName = QString("%1.%2").arg(m_logPath).arg(i);
            QString newName = QString("%1.%2").arg(m_logPath).arg(i + 1);
            QFile::rename(oldName, newName);
        }

        // Rename current file
        QFile::rename(m_logPath, m_logPath + ".1");
    }
}

bool ProxyLogger::openLogFile(QFile& file)
{
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Failed to open log file:" << m_logPath;
        return false;
    }
    return true;
} 