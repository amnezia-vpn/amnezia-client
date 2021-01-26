#ifndef UTILS_H
#define UTILS_H

#include <QString>

class Utils {

public:
    static QString configPath();
    static QString defaultVpnConfigFileName();
    static QString executable(const QString& baseName, bool absPath);
    static QString serverName();
    static QString systemLogPath();
    static QString toString(bool value);
    static bool createEmptyFile(const QString& path);
    static bool initializePath(const QString& path);
    static bool processIsRunning(const QString& fileName);

    static QString getIPAddress(const QString& host);
    static QString getStringBetween(const QString& s, const QString& a, const QString& b);
    static bool checkIPFormat(const QString& ip);
};

#endif // UTILS_H
