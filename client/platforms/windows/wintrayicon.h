#ifndef WINTRAYICON_H
#define WINTRAYICON_H

#include "core/protocols/vpnProtocol.h"

#include <QColor>
#include <QIcon>
#include <QSystemTrayIcon>

class QMenu;
class QString;

namespace WinTrayIcon
{
QIcon buildIcon(qreal opacity, bool darkTheme, const QColor &indicatorColor);
void applyTo(QSystemTrayIcon &trayIcon, Vpn::ConnectionState state, bool darkTheme);
QIcon buildNotifyIcon(bool darkTheme);

void configure(QSystemTrayIcon &trayIcon, QMenu *menu, const QString &tooltip);
void show(QSystemTrayIcon &trayIcon);
void showMessage(QSystemTrayIcon &trayIcon, const QString &title, const QString &message, bool darkTheme,
                 int timerMsec);
} // namespace WinTrayIcon

#endif // WINTRAYICON_H
