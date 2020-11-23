#ifndef DEBUG_H
#define DEBUG_H

#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDir>

class Debug
{
public:
    static bool init();
    static QString getPathToClientLog();
    static QString getPathToLogsDir();

private:
    static bool init(QDir& appDir);

private:
    static QFile m_clientLog;
    static QTextStream m_clientLogTextStream;
    static QString m_clientLogName;

    friend void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // DEBUG_H
