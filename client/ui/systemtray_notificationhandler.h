/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SYSTEMTRAY_NOTIFICATIONHANDLER_H
#define SYSTEMTRAY_NOTIFICATIONHANDLER_H

#include "notificationhandler.h"

#include <QMenu>
#include <QSystemTrayIcon>

class SystemTrayNotificationHandler : public NotificationHandler {
 public:
  explicit SystemTrayNotificationHandler(QObject* parent);
  ~SystemTrayNotificationHandler();

 protected:
  virtual void notify(Message type, const QString& title,
                      const QString& message, int timerMsec) override;

 private:
  void updateIcon(const QString& icon);

  void updateContextMenu();

  void showHideWindow();

  void maybeActivated(QSystemTrayIcon::ActivationReason reason);

 private:
  QMenu m_menu;
  QSystemTrayIcon m_systemTrayIcon;

  QAction* m_statusLabel = nullptr;
  QAction* m_lastLocationLabel = nullptr;
  QAction* m_disconnectAction = nullptr;
  QAction* m_separator = nullptr;
  QAction* m_preferencesAction = nullptr;
  QAction* m_showHideLabel = nullptr;
  QAction* m_quitAction = nullptr;
  QMenu* m_helpMenu = nullptr;
};

#endif  // SYSTEMTRAY_NOTIFICATIONHANDLER_H
