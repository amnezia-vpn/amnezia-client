#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

#include "ui/property_helper.h"

class Debug : public QObject
{
    Q_OBJECT
    AUTO_PROPERTY(QString, sshLog)
    AUTO_PROPERTY(QString, allLog)

public:
    static Debug& Instance();

    static void appendSshLog(const QString &log);
    static void appendAllLog(const QString &log);


    static bool init();
    static void deInit();
    static bool openLogsFolder();
    static bool openServiceLogsFolder();
    static QString appLogFileNamePath();
    static void clearLogs();
    static void clearServiceLogs();
    static void cleanUp();

    static QString userLogsFilePath();
    static QString getLogFile();

private:
    Debug() {}
    Debug(Debug const &) = delete;
    Debug& operator= (Debug const&) = delete;

    static QString userLogsDir();

    static QFile m_file;
    static QTextStream m_textStream;
    static QString m_logFileName;

    friend void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

#endif // DEBUG_H
