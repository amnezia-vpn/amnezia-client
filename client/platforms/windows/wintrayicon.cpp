#include "wintrayicon.h"

#include "ui/utils/trayIconCommon.h"

#include <QMenu>

namespace WinTrayIcon
{
QIcon buildIcon(qreal opacity, bool darkTheme, const QColor &indicatorColor)
{
    return TrayIconCommon::buildIcon(opacity, darkTheme, indicatorColor);
}

void applyTo(QSystemTrayIcon &trayIcon, Vpn::ConnectionState state, bool darkTheme)
{
    const qreal opacity = TrayIconCommon::opacityForState(state);
    const QColor indicatorColor = TrayIconCommon::indicatorColorForState(state);
    trayIcon.setIcon(buildIcon(opacity, darkTheme, indicatorColor));
}

QIcon buildNotifyIcon(bool darkTheme)
{
    return buildIcon(TrayIconCommon::kConnectedOpacity, darkTheme,
                     TrayIconCommon::indicatorColorForState(Vpn::ConnectionState::Connected));
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
