#ifndef NE_NOTIFICATION_HANDLER_H
#define NE_NOTIFICATION_HANDLER_H

#include "notificationhandler.h"
#include <QMenu>
#include <QAction>

class MacOSStatusIcon;

class NEStatusBarNotificationHandler : public NotificationHandler {
    Q_OBJECT
public:
    explicit NEStatusBarNotificationHandler(QObject* parent);
    ~NEStatusBarNotificationHandler() override;

    void setConnectionState(Vpn::ConnectionState state) override;
    void onTranslationsUpdated() override;

protected:
    void notify(Message type, const QString& title,
                const QString& message, int timerMsec) override;

private:
    void buildMenu();

    QMenu m_menu;
    MacOSStatusIcon* m_statusIcon;

    QAction* m_actionShow;
    QAction* m_actionConnect;
    QAction* m_actionDisconnect;
    QAction* m_actionVisitWebsite;
    QAction* m_actionQuit;
};

#endif // NE_NOTIFICATION_HANDLER_H
