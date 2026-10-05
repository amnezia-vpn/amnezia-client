#include "platformTrayTheme.h"

#include <QtGlobal>

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
#  include "platforms/macos/mactraytheme.h"
#elif defined(Q_OS_WIN) || (defined(Q_OS_MAC) && defined(MACOS_NE))
#  include "platforms/windows/wintraytheme.h"
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#  include "platforms/linux/linuxtraytheme.h"
#endif

void installTrayThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent)
{
    if (!onThemeChanged || !parent) {
        return;
    }

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
    MacTrayTheme::installThemeObserver(onThemeChanged, parent);
#elif defined(Q_OS_WIN) || (defined(Q_OS_MAC) && defined(MACOS_NE))
    WinTrayTheme::installThemeObserver(onThemeChanged, parent);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    LinuxTrayTheme::installThemeObserver(onThemeChanged, parent);
#endif
}
