/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxdependencies.h"
#include "dbusclient.h"
#include "logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include <mntent.h>

constexpr const char* WG_QUICK = "wg-quick";

namespace {

Logger logger(LOG_LINUX, "LinuxDependencies");

void showAlert(const QString& message) {
  logger.debug() << "Show alert:" << message;

  QMessageBox alert;
  alert.setText(message);
  alert.exec();
}

bool findInPath(const char* what) {
  char* path = getenv("PATH");
  Q_ASSERT(path);

  QStringList parts = QString(path).split(":");
  for (const QString& part : parts) {
    QDir pathDir(part);
    QFileInfo file(pathDir.filePath(what));
    if (file.exists()) {
      logger.debug() << what << "found" << file.filePath();
      return true;
    }
  }

  return false;
}

bool checkDaemonVersion() {
  logger.debug() << "Check Daemon Version";

  DBusClient* dbus = new DBusClient(nullptr);
  QDBusPendingCallWatcher* watcher = dbus->version();

  bool completed = false;
  bool value = false;
  QObject::connect(
      watcher, &QDBusPendingCallWatcher::finished,
      [completed = &completed, value = &value](QDBusPendingCallWatcher* call) {
        *completed = true;

        QDBusPendingReply<QString> reply = *call;
        if (reply.isError()) {
          logger.error() << "DBus message received - error";
          *value = false;
          return;
        }

        QString version = reply.argumentAt<0>();
        *value = version == PROTOCOL_VERSION;

        logger.debug() << "DBus message received - daemon version:" << version
                       << " - current version:" << PROTOCOL_VERSION;
      });

  while (!completed) {
    QCoreApplication::processEvents();
  }

  delete dbus;
  return value;
}

}  // namespace

// static
bool LinuxDependencies::checkDependencies() {
  char* path = getenv("PATH");
  if (!path) {
    showAlert("No PATH env found.");
    return false;
  }

  if (!findInPath(WG_QUICK)) {
    showAlert("Unable to locate wg-quick");
    return false;
  }

  if (!checkDaemonVersion()) {
    showAlert("mozillavpn linuxdaemon needs to be updated or restarted.");
    return false;
  }

  return true;
}

// static
QString LinuxDependencies::findCgroupPath(const QString& type) {
  struct mntent entry;
  char buf[PATH_MAX];

  FILE* fp = fopen("/etc/mtab", "r");
  if (fp == NULL) {
    return QString();
  }

  while (getmntent_r(fp, &entry, buf, sizeof(buf)) != NULL) {
    if (strcmp(entry.mnt_type, "cgroup") != 0) {
      continue;
    }
    if (hasmntopt(&entry, type.toLocal8Bit().constData()) != NULL) {
      fclose(fp);
      return QString(entry.mnt_dir);
    }
  }
  fclose(fp);

  return QString();
}
