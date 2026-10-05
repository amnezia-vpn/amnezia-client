#include "mactraytheme.h"

#include <QObject>

void MacTrayTheme::installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent)
{
    Q_UNUSED(onThemeChanged);
    Q_UNUSED(parent);
    // macOS template tray icons follow the menu bar appearance automatically.
}
