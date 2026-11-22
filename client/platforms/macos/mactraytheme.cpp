#include "mactraytheme.h"

#include <QObject>

void MacTrayTheme::installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent)
{
    Q_UNUSED(onThemeChanged);
    Q_UNUSED(parent);
}
