/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "platforms/linux/linuxsystemtraynotificationhandler.h"
#include "constants.h"
#include "leakdetector.h"
#include "logger.h"

#include <QtDBus/QtDBus>

constexpr const char* DBUS_ITEM = "org.freedesktop.Notifications";
constexpr const char* DBUS_PATH = "/org/freedesktop/Notifications";
constexpr const char* DBUS_INTERFACE = "org.freedesktop.Notifications";
constexpr const char* ACTION_ID = "mozilla_vpn_notification";

namespace {
Logger logger(LOG_LINUX, "LinuxSystemTrayNotificationHandler");
}  // namespace

// static
bool LinuxSystemTrayNotificationHandler::requiredCustomImpl() {
  if (!QDBusConnection::sessionBus().isConnected()) {
    return false;
  }

  QDBusConnectionInterface* interface =
      QDBusConnection::sessionBus().interface();
  if (!interface) {
    return false;
  }

  // This custom systemTrayHandler implementation is required only on Unity.
  QStringList registeredServices = interface->registeredServiceNames().value();
  return registeredServices.contains("com.canonical.Unity");
}

LinuxSystemTrayNotificationHandler::LinuxSystemTrayNotificationHandler(
    QObject* parent)
    : SystemTrayNotificationHandler(parent) {
  MVPN_COUNT_CTOR(LinuxSystemTrayNotificationHandler);

  QDBusConnection::sessionBus().connect(DBUS_ITEM, DBUS_PATH, DBUS_INTERFACE,
                                        "ActionInvoked", this,
                                        SLOT(actionInvoked(uint, QString)));
}

LinuxSystemTrayNotificationHandler::~LinuxSystemTrayNotificationHandler() {
  MVPN_COUNT_DTOR(LinuxSystemTrayNotificationHandler);
}

void LinuxSystemTrayNotificationHandler::notify(Message type,
                                                const QString& title,
                                                const QString& message,
                                                int timerMsec) {
  QString actionMessage;
  switch (type) {
    case None:
      return SystemTrayNotificationHandler::notify(type, title, message,
                                                   timerMsec);

    case UnsecuredNetwork:
      actionMessage = qtTrId("vpn.toggle.on");
      break;

    case CaptivePortalBlock:
      actionMessage = qtTrId("vpn.toggle.off");
      break;

    case CaptivePortalUnblock:
      actionMessage = qtTrId("vpn.toggle.on");
      break;

    default:
      Q_ASSERT(false);
  }

  m_lastMessage = type;
  emit notificationShown(title, message);

  QDBusInterface n(DBUS_ITEM, DBUS_PATH, DBUS_INTERFACE,
                   QDBusConnection::sessionBus());
  if (!n.isValid()) {
    qWarning("Failed to connect to the notification manager via system dbus");
    return;
  }

  uint32_t replacesId = 0;  // Don't replace.
  const char* appIcon = MVPN_ICON_PATH;
  QStringList actions{ACTION_ID, actionMessage};
  QMap<QString, QVariant> hints;

  QDBusReply<uint> reply = n.call("Notify", "Mozilla VPN", replacesId, appIcon,
                                  title, message, actions, hints, timerMsec);
  if (!reply.isValid()) {
    logger.warning() << "Failed to show the notification";
  }

  m_lastNotificationId = reply;
}

void LinuxSystemTrayNotificationHandler::actionInvoked(uint actionId,
                                                       QString action) {
  logger.debug() << "Notification clicked" << actionId << action;

  if (action == ACTION_ID && m_lastNotificationId == actionId) {
    messageClickHandle();
  }
}
