/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "platforms/linux/linuxsystemtraynotificationhandler.h"
#include "constants.h"
#include "leakdetector.h"
#include "logger.h"
#include "defines.h"

#include <QtDBus/QtDBus>
#include <QDesktopServices>

constexpr const char* DBUS_ITEM = "org.freedesktop.Notifications";
constexpr const char* DBUS_PATH = "/org/freedesktop/Notifications";
constexpr const char* DBUS_INTERFACE = "org.freedesktop.Notifications";
constexpr const char* ACTION_ID = "mozilla_vpn_notification";

namespace {
Logger logger(LOG_LINUX, "LinuxSystemTrayNotificationHandler");
}  // namespace

//static
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

    m_systemTrayIcon.show();
    connect(&m_systemTrayIcon, &QSystemTrayIcon::activated, this);


    m_menu.addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME, this, [this](){
        emit raiseRequested();
    });
    m_menu.addSeparator();
    m_trayActionConnect = m_menu.addAction(tr("Connect"), this, [this](){ emit connectRequested(); });
    m_trayActionDisconnect = m_menu.addAction(tr("Disconnect"), this, [this](){ emit disconnectRequested(); });

    m_menu.addSeparator();

    m_menu.addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl("https://amnezia.org"));
    });

    m_menu.addAction(QIcon(":/images/tray/cancel.png"), tr("Quit") + " " + APPLICATION_NAME, this, [&](){
        qApp->quit();
    });

    m_systemTrayIcon.setContextMenu(&m_menu);
    setTrayState(VpnProtocol::Disconnected);
}

LinuxSystemTrayNotificationHandler::~LinuxSystemTrayNotificationHandler() {
  MVPN_COUNT_DTOR(LinuxSystemTrayNotificationHandler);
}

void LinuxSystemTrayNotificationHandler::notify(Message type,
                                                const QString& title,
                                                const QString& message,
                                                int timerMsec) {


}

void LinuxSystemTrayNotificationHandler::actionInvoked(uint actionId,
                                                       QString action) {
  logger.debug() << "Notification clicked" << actionId << action;

  if (action == ACTION_ID && m_lastNotificationId == actionId) {
    messageClickHandle();
  }
}
