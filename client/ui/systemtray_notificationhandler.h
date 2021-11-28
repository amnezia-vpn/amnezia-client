/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SYSTEMTRAY_NOTIFICATIONHANDLER_H
#define SYSTEMTRAY_NOTIFICATIONHANDLER_H

#include "notificationhandler.h"

#include <QMenu>
#include <QSystemTrayIcon>

class SystemTrayNotificationHandler : public NotificationHandler {
    Q_OBJECT

public:
    explicit SystemTrayNotificationHandler(QObject* parent);
    ~SystemTrayNotificationHandler();

    void setConnectionState(VpnProtocol::ConnectionState state) override;

protected:
    virtual void notify(Message type, const QString& title,
                        const QString& message, int timerMsec) override;

private:
    void showHideWindow();

    void setTrayState(VpnProtocol::ConnectionState state);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void setTrayIcon(const QString &iconPath);

private:
    QMenu m_menu;
    QSystemTrayIcon m_systemTrayIcon;

    QAction* m_trayActionConnect = nullptr;
    QAction* m_trayActionDisconnect = nullptr;
    QAction* m_preferencesAction = nullptr;
    QAction* m_statusLabel = nullptr;    
    QAction* m_separator = nullptr;

    const QString ConnectedTrayIconName = "active.png";
    const QString DisconnectedTrayIconName = "default.png";
    const QString ErrorTrayIconName = "error.png";
};

#endif  // SYSTEMTRAY_NOTIFICATIONHANDLER_H
