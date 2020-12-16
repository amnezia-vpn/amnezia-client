#ifndef DEBUG_H
#define DEBUG_H

#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDir>

class Debug
{
public:
    static QString logsDir();
    static bool init();
    static bool openLogsFolder();

private:
    static QFile m_file;
    static QTextStream m_textStream;
    static QString m_logFileName;

    friend void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

#endif // DEBUG_H
