#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

#include "ui/property_helper.h"

#include "mozilla/shared/loglevel.h"

class Logger : public QObject
{
    Q_OBJECT
    AUTO_PROPERTY(QString, sshLog)
    AUTO_PROPERTY(QString, allLog)

public:
    static Logger& Instance();

    static void appendSshLog(const QString &log);
    static void appendAllLog(const QString &log);


    static bool init();
    static void deInit();
    static bool setServiceLogsEnabled(bool enabled);
    static bool openLogsFolder();
    static bool openServiceLogsFolder();
    static QString appLogFileNamePath();
    static void clearLogs();
    static void clearServiceLogs();
    static void cleanUp();

    static QString userLogsFilePath();
    static QString serviceLogsFilePath();

    static QString getLogFile();
    static QString getServiceLogFile();

    // compat with Mozilla logger
    Logger(const QString &className) { m_className = className; }
    const QString& className() const { return m_className; }

    class Log {
    public:
        Log(Logger* logger, LogLevel level);
        ~Log();

        Log& operator<<(uint64_t t);
        Log& operator<<(const char* t);
        Log& operator<<(const QString& t);
        Log& operator<<(const QStringList& t);
        Log& operator<<(const QByteArray& t);
        Log& operator<<(const QJsonObject& t);
        Log& operator<<(QTextStreamFunction t);
        Log& operator<<(const void* t);

        // Q_ENUM
        template <typename T>
        typename std::enable_if<QtPrivate::IsQEnumHelper<T>::Value, Log&>::type
        operator<<(T t) {
            const QMetaObject* meta = qt_getEnumMetaObject(t);
            const char* name = qt_getEnumName(t);
            addMetaEnum(typename QFlags<T>::Int(t), meta, name);
            return *this;
        }

    private:
        void addMetaEnum(quint64 value, const QMetaObject* meta, const char* name);

        Logger* m_logger;
        LogLevel m_logLevel;

        struct Data {
            Data() : m_ts(&m_buffer, QIODevice::WriteOnly) {}

            QString m_buffer;
            QTextStream m_ts;
        };

        Data* m_data;
    };

    Log error();
    Log warning();
    Log info();
    Log debug();
    QString sensitive(const QString& input);

private:
    Logger() {}
    Logger(Logger const &) = delete;
    Logger& operator= (Logger const&) = delete;

    static QString userLogsDir();

    static QFile m_file;
    static QTextStream m_textStream;
    static QString m_logFileName;
    static QString m_serviceLogFileName;

    friend void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    // compat with Mozilla logger
    QString m_className;
};

#endif // LOGGER_H
