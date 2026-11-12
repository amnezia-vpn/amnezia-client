#include "wintrayicon.h"

#include "ui/utils/trayIconCommon.h"

#include <QMenu>

namespace WinTrayIcon
{
QIcon buildIcon(Vpn::ConnectionState state, bool darkTheme)
{
    return TrayIconCommon::buildIcon(state, darkTheme);
}

void applyTo(QSystemTrayIcon &trayIcon, Vpn::ConnectionState state, bool darkTheme)
{
    trayIcon.setIcon(buildIcon(state, darkTheme));
}

QIcon buildNotifyIcon(bool darkTheme)
{
    return buildIcon(Vpn::ConnectionState::Connected, darkTheme);
}

void configure(QSystemTrayIcon &trayIcon, QMenu *menu, const QString &tooltip)
{
    trayIcon.setContextMenu(menu);
    trayIcon.setToolTip(tooltip);
}

void show(QSystemTrayIcon &trayIcon)
{
    trayIcon.show();
}

void showMessage(QSystemTrayIcon &trayIcon, const QString &title, const QString &message, bool darkTheme,
                 int timerMsec)
{
    trayIcon.showMessage(title, message, buildNotifyIcon(darkTheme), timerMsec);
}
} // namespace WinTrayIcon
