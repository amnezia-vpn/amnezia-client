#include "wintrayiconbackend.h"

#include "platforms/windows/wintrayicon.h"

#include <QObject>

WinTrayIconBackend::WinTrayIconBackend(QObject *parent) : m_trayIcon(parent)
{
    m_reapplyTimerShort.setSingleShot(true);
    m_reapplyTimerLong.setSingleShot(true);
    QObject::connect(&m_reapplyTimerShort, &QTimer::timeout, &m_trayIcon, [this]() { reapplyLastVisual(); });
    QObject::connect(&m_reapplyTimerLong, &QTimer::timeout, &m_trayIcon, [this]() { reapplyLastVisual(); });
}

void WinTrayIconBackend::reapplyLastVisual()
{
    WinTrayIcon::applyTo(m_trayIcon, m_lastVisual.connectionState, m_lastVisual.darkTheme);
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
    m_lastVisual = visual;
    WinTrayIcon::applyTo(m_trayIcon, visual.connectionState, visual.darkTheme);

    m_reapplyTimerShort.start(250);
    m_reapplyTimerLong.start(1200);
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
