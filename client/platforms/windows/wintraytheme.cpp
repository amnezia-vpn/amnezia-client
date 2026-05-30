#include "wintraytheme.h"

#include "platforms/windows/windowsutils.h"
#include "ui/utils/trayThemeChangeFilter.h"

#include <QApplication>
#include <QGuiApplication>
#include <QObject>
#include <QStyleHints>

void WinTrayTheme::installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent)
{
    if (!onThemeChanged || !parent) {
        return;
    }

    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, parent, [onThemeChanged]() { onThemeChanged(); });
    }

    qApp->installEventFilter(new TrayThemeChangeFilter(onThemeChanged, parent));

#if defined(Q_OS_WIN)
    WindowsUtils::installThemeChangeObserver(onThemeChanged);
#endif
}
