#ifndef UTILITIES_H
#define UTILITIES_H

#include <QRegExp>
#include <QRegularExpression>
#include <QString>
#include <QJsonDocument>

#ifdef Q_OS_WIN
    #include "Windows.h"
#endif

class Utils : public QObject
{
    Q_OBJECT

public:
    static QString getRandomString(int len);
    static QString SafeBase64Decode(QString string);
    static QString VerifyJsonString(const QString &source);
    static QString JsonToString(const QJsonObject &json, QJsonDocument::JsonFormat format);
    static QString JsonToString(const QJsonArray &array, QJsonDocument::JsonFormat format);
    static QJsonObject JsonFromString(const QString &string);
    static QString executable(const QString &baseName, bool absPath);
    static QString usrExecutable(const QString &baseName);
    static QString systemLogPath();
    static bool createEmptyFile(const QString &path);
    static bool initializePath(const QString &path);

    static bool processIsRunning(const QString &fileName, const bool fullFlag = false);
    static void killProcessByName(const QString &name);

    static QString openVpnExecPath();
    static QString wireguardExecPath();
    static QString certUtilPath();
    static QString tun2socksPath();
    static QString goodbyedpiPath();

#ifdef Q_OS_WIN
    static bool signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent);
#endif
};

#endif // UTILITIES_H
