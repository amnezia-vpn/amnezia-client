#ifndef UTILS_H
#define UTILS_H

#include <QRegExp>
#include <QString>

#ifdef Q_OS_WIN
#include "Windows.h"
#endif

class Utils {

public:
    static QString getRandomString(int len);

    static QString configPath();
    static QString defaultVpnConfigFileName();
    static QString executable(const QString& baseName, bool absPath);
    static QString systemLogPath();
    static bool createEmptyFile(const QString& path);
    static bool initializePath(const QString& path);

    static QString getIPAddress(const QString& host);
    static QString getStringBetween(const QString& s, const QString& a, const QString& b);
    static bool checkIPFormat(const QString& ip);
    static QRegExp ipAddressRegExp() { return QRegExp("^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\\.(?!$)|$)){4}$"); }
    static QRegExp ipAddressPortRegExp() { return QRegExp("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
        "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])(\\:[0-9]{1,5}){0,1}$"); }

    static bool processIsRunning(const QString& fileName);
    static void killProcessByName(const QString &name);

#ifdef Q_OS_WIN
    static bool signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent);
#endif
};

#endif // UTILS_H
