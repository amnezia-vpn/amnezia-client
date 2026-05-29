/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SYSTEMTRAYNOTIFICATIONHANDLER_H
#define SYSTEMTRAYNOTIFICATIONHANDLER_H

#include "notificationHandler.h"

#include <QColor>
#include <QMenu>
#include <QSystemTrayIcon>

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
class MacOSStatusIcon;
#endif

class SystemTrayNotificationHandler : public NotificationHandler {
    Q_OBJECT

public:
    explicit SystemTrayNotificationHandler(QObject* parent);
    ~SystemTrayNotificationHandler();

    void setConnectionState(Vpn::ConnectionState state) override;

    void onTranslationsUpdated() override;

public slots:
    void updateWebsiteUrl(const QString &newWebsiteUrl);

protected:
    virtual void notify(Message type, const QString& title,
                        const QString& message, int timerMsec) override;

private:
    void showHideWindow();

    void setTrayState(Vpn::ConnectionState state);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void refreshTheme();
    void updateTrayIcon();
    qreal trayIconOpacityForState(Vpn::ConnectionState state) const;
    QColor trayIndicatorColorForState(Vpn::ConnectionState state) const;

private:
    QMenu m_menu;

#if defined(Q_OS_MAC) && !defined(MACOS_NE)
    MacOSStatusIcon *m_macStatusIcon = nullptr;
#else
    QSystemTrayIcon m_systemTrayIcon;
#endif

    QAction* m_trayActionShow = nullptr;
    QAction* m_trayActionConnect = nullptr;
    QAction* m_trayActionDisconnect = nullptr;
    QAction* m_trayActionVisitWebSite = nullptr;
    QAction* m_trayActionQuit = nullptr;
    QAction* m_statusLabel = nullptr;
    QAction* m_separator = nullptr;

    Vpn::ConnectionState m_trayState = Vpn::ConnectionState::Unknown;
    bool m_isDarkTheme = false;

    static constexpr qreal kDisconnectedTrayOpacity = 0.5;
    static constexpr qreal kConnectedTrayOpacity = 1.0;

    QString websiteUrl = "https://amnezia.org";
};

#endif  // SYSTEMTRAYNOTIFICATIONHANDLER_H
