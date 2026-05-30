#include "linuxtraytheme.h"

#include "platforms/linux/linuxutils.h"

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

    LinuxUtils::installThemeChangeObserver(onThemeChanged);
}
