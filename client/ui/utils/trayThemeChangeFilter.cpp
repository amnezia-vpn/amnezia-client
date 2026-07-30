#include "trayThemeChangeFilter.h"

#include <QEvent>

TrayThemeChangeFilter::TrayThemeChangeFilter(std::function<void()> onThemeChanged, QObject *parent)
    : QObject(parent)
    , m_onThemeChanged(std::move(onThemeChanged))
{
}

bool TrayThemeChangeFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ThemeChange) {
        if (m_onThemeChanged) {
            m_onThemeChanged();
        }
    }
    return QObject::eventFilter(watched, event);
}
