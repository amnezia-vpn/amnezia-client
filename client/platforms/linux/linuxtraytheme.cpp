#include "linuxtraytheme.h"

#include "platforms/linux/linuxutils.h"
#include "ui/utils/trayThemeChangeFilter.h"

#include <QApplication>
#include <QGuiApplication>
#include <QObject>
#include <QStyleHints>

void LinuxTrayTheme::installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent)
{
    if (!onThemeChanged || !parent) {
        return;
    }

    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, parent, [onThemeChanged]() { onThemeChanged(); });
    }

    qApp->installEventFilter(new TrayThemeChangeFilter(onThemeChanged, parent));

    LinuxUtils::installThemeChangeObserver(onThemeChanged);
}
