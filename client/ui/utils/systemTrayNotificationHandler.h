/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SYSTEMTRAYNOTIFICATIONHANDLER_H
#define SYSTEMTRAYNOTIFICATIONHANDLER_H

#include "notificationHandler.h"
#include "trayIconBackend.h"

#include <QMenu>

#include <memory>

class SystemTrayNotificationHandler : public NotificationHandler {
    Q_OBJECT

public:
    explicit SystemTrayNotificationHandler(QObject* parent);
    ~SystemTrayNotificationHandler() override;

    void setConnectionState(Vpn::ConnectionState state) override;

    void onTranslationsUpdated() override;

public slots:
    void updateWebsiteUrl(const QString &newWebsiteUrl);

protected:
    void notify(Message type, const QString& title,
                const QString& message, int timerMsec) override;

private:
    void setTrayState(Vpn::ConnectionState state);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void refreshTheme();
    void updateTrayIcon();
    TrayIconVisual currentTrayVisual() const;

private:
    QMenu m_menu;
    std::unique_ptr<TrayIconBackend> m_trayIcon;

    QAction* m_trayActionShow = nullptr;
    QAction* m_trayActionConnect = nullptr;
    QAction* m_trayActionDisconnect = nullptr;
    QAction* m_trayActionVisitWebSite = nullptr;
    QAction* m_trayActionQuit = nullptr;
    QAction* m_statusLabel = nullptr;
    QAction* m_separator = nullptr;

    Vpn::ConnectionState m_trayState = Vpn::ConnectionState::Unknown;
    bool m_isDarkTheme = false;

    QString websiteUrl = "https://amnezia.org";
};

#endif  // SYSTEMTRAYNOTIFICATIONHANDLER_H
