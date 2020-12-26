#ifndef UTILS_H
#define UTILS_H

#include <QString>

class Utils {

public:
    static QString executable(const QString& baseName, bool absPath);
    static QString systemConfigPath();
    static QString systemDataLocationPath();
    static QString systemLogPath();
    static QString toString(bool value);
    static bool createEmptyFile(const QString& path);
    static bool processIsRunning(const QString& fileName);
};

#endif // UTILS_H
