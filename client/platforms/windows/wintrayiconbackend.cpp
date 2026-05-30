#include "wintrayiconbackend.h"

#include "platforms/windows/wintrayicon.h"

#include <QObject>

WinTrayIconBackend::WinTrayIconBackend(QObject *parent) : m_trayIcon(parent)
{
}

void WinTrayIconBackend::setMenu(QMenu *menu)
{
    m_trayIcon.setContextMenu(menu);
}

void WinTrayIconBackend::setToolTip(const QString &tooltip)
{
    m_trayIcon.setToolTip(tooltip);
}

void WinTrayIconBackend::show()
{
    WinTrayIcon::show(m_trayIcon);
}

void WinTrayIconBackend::applyVisual(const TrayIconVisual &visual)
{
    WinTrayIcon::applyTo(m_trayIcon, visual.connectionState, visual.darkTheme);
}

void WinTrayIconBackend::showMessage(const QString &title, const QString &message, const TrayIconVisual &visual,
                                     int timerMsec)
{
    WinTrayIcon::showMessage(m_trayIcon, title, message, visual.darkTheme, timerMsec);
}

void WinTrayIconBackend::rebuildMenu()
{
}

void WinTrayIconBackend::setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler)
{
    if (!handler) {
        return;
    }

    QObject::connect(&m_trayIcon, &QSystemTrayIcon::activated, m_trayIcon.parent(),
                     [handler](QSystemTrayIcon::ActivationReason reason) { handler(reason); });
}

std::unique_ptr<TrayIconBackend> createTrayIconBackend(QObject *parent)
{
    return std::make_unique<WinTrayIconBackend>(parent);
}
