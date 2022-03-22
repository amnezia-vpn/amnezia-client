/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "apptracker.h"
#include "dbustypeslinux.h"
#include "leakdetector.h"
#include "logger.h"

#include <QtDBus/QtDBus>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QScopeGuard>

#include <unistd.h>

constexpr const char* GTK_DESKTOP_APP_SERVICE = "org.gtk.gio.DesktopAppInfo";
constexpr const char* GTK_DESKTOP_APP_PATH = "/org/gtk/gio/DesktopAppInfo";

constexpr const char* DBUS_LOGIN_SERVICE = "org.freedesktop.login1";
constexpr const char* DBUS_LOGIN_PATH = "/org/freedesktop/login1";
constexpr const char* DBUS_LOGIN_MANAGER = "org.freedesktop.login1.Manager";

namespace {
Logger logger(LOG_LINUX, "AppTracker");
}

AppTracker::AppTracker(QObject* parent) : QObject(parent) {
  MVPN_COUNT_CTOR(AppTracker);
  logger.debug() << "AppTracker created.";

  QDBusConnection m_conn = QDBusConnection::systemBus();
  m_conn.connect("", DBUS_LOGIN_PATH, DBUS_LOGIN_MANAGER, "UserNew", this,
                 SLOT(userCreated(uint, const QDBusObjectPath&)));
  m_conn.connect("", DBUS_LOGIN_PATH, DBUS_LOGIN_MANAGER, "UserRemoved", this,
                 SLOT(userRemoved(uint, const QDBusObjectPath&)));

  QDBusInterface n(DBUS_LOGIN_SERVICE, DBUS_LOGIN_PATH, DBUS_LOGIN_MANAGER,
                   m_conn);
  QDBusPendingReply<UserDataList> reply = n.asyncCall("ListUsers");
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this,
                   SLOT(userListCompleted(QDBusPendingCallWatcher*)));
}

AppTracker::~AppTracker() {
  MVPN_COUNT_DTOR(AppTracker);
  logger.debug() << "AppTracker destroyed.";
}

void AppTracker::userListCompleted(QDBusPendingCallWatcher* watcher) {
  QDBusPendingReply<UserDataList> reply = *watcher;
  if (reply.isValid()) {
    UserDataList list = reply.value();
    for (auto user : list) {
      userCreated(user.userid, user.path);
    }
  }

  delete watcher;
}

void AppTracker::userCreated(uint userid, const QDBusObjectPath& path) {
  logger.debug() << "User created uid:" << userid << "at:" << path.path();

  /* Acquire the effective UID of the user to connect to their session bus. */
  uid_t realuid = getuid();
  if (seteuid(userid) < 0) {
    logger.warning() << "Failed to set effective UID";
  }
  auto guard = qScopeGuard([&] {
    if (seteuid(realuid) < 0) {
      logger.warning() << "Failed to restore effective UID";
    }
  });

  /* For correctness we should ask systemd for the user's runtime directory. */
  QString busPath = "unix:path=/run/user/" + QString::number(userid) + "/bus";
  logger.debug() << "Connection to" << busPath;
  QDBusConnection connection =
      QDBusConnection::connectToBus(busPath, "user-" + QString::number(userid));

  /* Connect to the user's GTK launch event. */
  bool isConnected = connection.connect(
      "", GTK_DESKTOP_APP_PATH, GTK_DESKTOP_APP_SERVICE, "Launched", this,
      SLOT(gtkLaunchEvent(const QByteArray&, const QString&, qlonglong,
                          const QStringList&, const QVariantMap&)));
  if (!isConnected) {
    logger.warning() << "Failed to connect to GTK Launched signal";
  }
}

void AppTracker::userRemoved(uint uid, const QDBusObjectPath& path) {
  logger.debug() << "User removed uid:" << uid << "at:" << path.path();
  QDBusConnection::disconnectFromBus("user-" + QString::number(uid));
}

void AppTracker::gtkLaunchEvent(const QByteArray& appid, const QString& display,
                                qlonglong pid, const QStringList& uris,
                                const QVariantMap& extra) {
  Q_UNUSED(display);
  Q_UNUSED(uris);
  Q_UNUSED(extra);

  QString appIdName(appid);
  if (!appIdName.isEmpty()) {
    emit appLaunched(appIdName, pid);
  }
}
