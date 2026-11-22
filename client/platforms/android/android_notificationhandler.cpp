#include "android_notificationhandler.h"

#include "android_controller.h"

AndroidNotificationHandler::AndroidNotificationHandler(QObject *parent)
    : NotificationHandler(parent)
{
}

void AndroidNotificationHandler::notify(Message type, const QString &title, const QString &message, int timerMsec)
{
    Q_UNUSED(type);
    Q_UNUSED(timerMsec);
    // Permission is checked on the Kotlin side as well; avoid JNI if already denied.
    if (!AndroidController::instance()->isNotificationPermissionGranted()) {
        return;
    }
    AndroidController::instance()->showVpnStateNotification(title, message);
}
