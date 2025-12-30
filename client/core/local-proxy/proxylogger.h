#ifndef LOCAL_PROXY_LOGGER_H
#define LOCAL_PROXY_LOGGER_H

#include <QObject>
#include <QFile>
#include <QDateTime>
#include <QMutex>
#include <QString>

class ProxyLogger
{

public:
    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    static ProxyLogger& getInstance();

    void init(const QString& logPath, qint64 maxFileSize = 1024 * 1024 * 10); // 10MB by default
    void setLogLevel(LogLevel level);

    // Main logging method
    void log(LogLevel level, const QString& message);

    // Helper methods for convenience
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

private:
    ProxyLogger();
    ~ProxyLogger();
    ProxyLogger(const ProxyLogger&) = delete;
    ProxyLogger& operator=(const ProxyLogger&) = delete;

    void logInternal(LogLevel level, const QString& message);
    void checkRotation();
    QString levelToString(LogLevel level);
    bool openLogFile(QFile& file);
    qint64 getCurrentFileSize() const;

    QString m_logPath;
    qint64 m_maxFileSize;
    LogLevel m_currentLevel;
    QMutex m_mutex;
    static const int MAX_BACKUP_FILES = 3;
};

#endif // LOCAL_PROXY_LOGGER_H 