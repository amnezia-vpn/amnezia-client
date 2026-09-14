#ifndef MACOSAPPINFO_H
#define MACOSAPPINFO_H

#include <QString>

#include "core/utils/commonStructs.h"

class MacOSAppInfo
{
public:
    static bool fillFromPath(const QString &appPath, amnezia::InstalledAppInfo &info);
    static QString pickApplication();
};

#endif
