#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

class Debug
{
public:
    static bool init();
    static bool openLogsFolder();
    static QString appLogFileNamePath();

private:
    static QString userLogsDir();

    static QFile m_file;
    static QTextStream m_textStream;
    static QString m_logFileName;

    friend void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

#endif // DEBUG_H
