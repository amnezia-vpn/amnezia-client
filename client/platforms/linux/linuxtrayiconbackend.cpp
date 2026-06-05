#include "linuxtrayiconbackend.h"

#include "ui/utils/trayIconCommon.h"

#include <QObject>

namespace
{

    constexpr int kLinuxTrayIconSizes[] = { 16, 22, 24, 32, 48, 64, 128 };

} // namespace

LinuxTrayIconBackend::LinuxTrayIconBackend(QObject *parent) : m_trayIcon(parent)
{
}

void LinuxTrayIconBackend::setMenu(QMenu *menu)
{
    m_trayIcon.setContextMenu(menu);
}

void LinuxTrayIconBackend::setToolTip(const QString &tooltip)
{
    m_trayIcon.setToolTip(tooltip);
}

void LinuxTrayIconBackend::show()
{
    m_trayIcon.show();
}

void LinuxTrayIconBackend::applyVisual(const TrayIconVisual &visual)
{
    const QIcon icon = buildTrayIcon(visual.connectionState, visual.darkTheme);

    // Some tray implementations cache the first icon; clear before applying an update.
    if (m_trayIcon.isVisible()) {
        m_trayIcon.setIcon(QIcon());
    }
    m_trayIcon.setIcon(icon);
}

void LinuxTrayIconBackend::showMessage(const QString &title, const QString &message, const TrayIconVisual &visual,
                                       int timerMsec)
{
    m_trayIcon.showMessage(title, message,
                           buildTrayIcon(Vpn::ConnectionState::Connected, visual.darkTheme),
                           timerMsec);
}

void LinuxTrayIconBackend::rebuildMenu()
{
}

void LinuxTrayIconBackend::setActivatedHandler(std::function<void(QSystemTrayIcon::ActivationReason)> handler)
{
    if (!handler) {
        return;
    }

    QObject::connect(&m_trayIcon, &QSystemTrayIcon::activated, m_trayIcon.parent(),
                     [handler](QSystemTrayIcon::ActivationReason reason) { handler(reason); });
}

QIcon LinuxTrayIconBackend::buildTrayIcon(Vpn::ConnectionState state, bool darkTheme) const
{
    QIcon icon;
    for (int size : kLinuxTrayIconSizes) {
        icon.addPixmap(TrayIconCommon::buildPixmap(size, state, darkTheme));
    }
    return icon;
}

std::unique_ptr<TrayIconBackend> createTrayIconBackend(QObject *parent)
{
    return std::make_unique<LinuxTrayIconBackend>(parent);
}
