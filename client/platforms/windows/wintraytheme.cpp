#include "wintraytheme.h"

#include "platforms/windows/windowsutils.h"
#include "ui/utils/trayThemeChangeFilter.h"

#include <QApplication>
#include <QGuiApplication>
#include <QObject>
#include <QStyleHints>
#include <QTimer>

void WinTrayTheme::installThemeObserver(const std::function<void()> &onThemeChanged, QObject *parent)
{
    if (!onThemeChanged || !parent) {
        return;
    }

    auto *debounce = new QTimer(parent);
    debounce->setSingleShot(true);
    QObject::connect(debounce, &QTimer::timeout, parent, [onThemeChanged]() { onThemeChanged(); });

    const auto schedule = [debounce]() { debounce->start(150); };

    if (QStyleHints *styleHints = QGuiApplication::styleHints()) {
        QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, parent,
                         [schedule](Qt::ColorScheme) { schedule(); });
    }

    qApp->installEventFilter(new TrayThemeChangeFilter([schedule]() { schedule(); }, parent));

    WindowsUtils::installThemeChangeObserver([schedule]() { schedule(); });
}
