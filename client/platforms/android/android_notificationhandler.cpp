/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "android_notificationhandler.h"
#include "platforms/android/android_controller.h"

AndroidNotificationHandler::AndroidNotificationHandler(QObject* parent)
    : NotificationHandler(parent) {
}
AndroidNotificationHandler::~AndroidNotificationHandler() {
}

void AndroidNotificationHandler::notify(NotificationHandler::Message type,
                                        const QString& title,
                                        const QString& message, int timerMsec) {
  Q_UNUSED(type);
  qDebug() << "Send notification - " << message;
  AndroidController::instance()->setNotificationText(title, message,
                                                     timerMsec / 1000);
}
