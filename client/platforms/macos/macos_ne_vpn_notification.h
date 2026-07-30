#ifndef MACOS_NE_VPN_NOTIFICATION_H
#define MACOS_NE_VPN_NOTIFICATION_H

class QString;

void macosNePostVpnStateNotification(const QString &title, const QString &message);

#endif
