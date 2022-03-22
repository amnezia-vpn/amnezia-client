/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXNOTIFICATIONNOTIFICATIONHANDLER_H
#define LINUXNOTIFICATIONNOTIFICATIONHANDLER_H

#include "./ui/systemtray_notificationhandler.h"

#include <QObject>

class LinuxSystemTrayNotificationHandler final
    : public SystemTrayNotificationHandler {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LinuxSystemTrayNotificationHandler)

 public:
  static bool requiredCustomImpl();

  LinuxSystemTrayNotificationHandler(QObject* parent);
  ~LinuxSystemTrayNotificationHandler();

 private:
  void notify(Message type, const QString& title, const QString& message,
              int timerMsec) override;

 private slots:
  void actionInvoked(uint actionId, QString action);

 private:
  uint m_lastNotificationId = 0;
};

#endif  // LINUXNOTIFICATIONNOTIFICATIONHANDLER_H
