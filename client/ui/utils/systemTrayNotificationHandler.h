/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SYSTEMTRAYNOTIFICATIONHANDLER_H
#define SYSTEMTRAYNOTIFICATIONHANDLER_H

#include "notificationHandler.h"

#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTimer>

class MacOSStatusIcon;

class SystemTrayNotificationHandler : public NotificationHandler {
    Q_OBJECT

public:
    explicit SystemTrayNotificationHandler(QObject* parent);
    ~SystemTrayNotificationHandler();

    void setConnectionState(Vpn::ConnectionState state) override;

    void onTranslationsUpdated() override;

public slots:
    void setConnectedTrayIconColor(const QColor &color);
    void updateWebsiteUrl(const QString &newWebsiteUrl);

protected:
    virtual void notify(Message type, const QString& title,
                        const QString& message, int timerMsec) override;

private:
    void showHideWindow();

    void setTrayState(Vpn::ConnectionState state);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    QColor trayIconColor() const;
    void updateTrayIcon();
    void setTrayIcon(const QImage &icon, bool isTemplate);
    void updateTrayToolTip();

private:
    QMenu m_menu;
#ifdef Q_OS_MAC
    MacOSStatusIcon* m_statusIcon = nullptr;
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

    QColor m_connectedTrayIconColor = QColor(0xFD, 0xBA, 0x74);
    const QColor ErrorTrayIconColor = QColor(0xF8, 0x71, 0x71);
    const int PulsePeriodMs = 1600;
    QImage m_trayIconBase;
    QColor m_trayIconColor;
    QTimer m_pulseTimer;
    QElapsedTimer m_pulseClock;
    Vpn::ConnectionState m_trayState = Vpn::ConnectionState::Disconnected;
    QString  websiteUrl = "https://amnezia.org";
};

#endif  // SYSTEMTRAYNOTIFICATIONHANDLER_H
