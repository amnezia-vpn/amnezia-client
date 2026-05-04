#ifndef ANDROID_NOTIFICATIONHANDLER_H
#define ANDROID_NOTIFICATIONHANDLER_H

#include "ui/notificationhandler.h"

class AndroidNotificationHandler final : public NotificationHandler {
    Q_OBJECT

public:
    explicit AndroidNotificationHandler(QObject *parent = nullptr);

protected:
    void notify(Message type, const QString &title, const QString &message, int timerMsec) override;
};

#endif // ANDROID_NOTIFICATIONHANDLER_H
