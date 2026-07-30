#include "platformTheme.h"

#include <QGuiApplication>
#include <QStyleHints>

#if defined(Q_OS_MAC)
#  include "platforms/macos/macosutils.h"
#elif defined(Q_OS_WIN)
#  include "platforms/windows/windowsutils.h"
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#  include "platforms/linux/linuxutils.h"
#endif

bool platformIsDarkTheme()
{
#if defined(Q_OS_MAC)
    return MacOSUtils::isDarkTheme();
#elif defined(Q_OS_WIN)
    return WindowsUtils::isDarkTheme();
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    return LinuxUtils::isDarkTheme();
#else
    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        return styleHints->colorScheme() == Qt::ColorScheme::Dark;
    }
    return false;
#endif
}
