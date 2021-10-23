/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IOSNOTIFICATIONHANDLER_H
#define IOSNOTIFICATIONHANDLER_H

#include "notificationhandler.h"

#include <QObject>

class IOSNotificationHandler final : public NotificationHandler {
  Q_DISABLE_COPY_MOVE(IOSNotificationHandler)

 public:
  IOSNotificationHandler(QObject* parent);
  ~IOSNotificationHandler();

 protected:
  void notify(const QString& title, const QString& message,
              int timerSec) override;

 private:
  void* m_delegate = nullptr;
};

#endif  // IOSNOTIFICATIONHANDLER_H
