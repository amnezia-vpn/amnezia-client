/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxapplistprovider.h"
#include "leakdetector.h"

#include <QProcess>
#include <QString>
#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QProcessEnvironment>

#include "logger.h"
#include "leakdetector.h"

constexpr const char* DESKTOP_ENTRY_LOCATION = "/usr/share/applications/";

namespace {
Logger logger(LOG_CONTROLLER, "LinuxAppListProvider");
}

LinuxAppListProvider::LinuxAppListProvider(QObject* parent)
    : AppListProvider(parent) {
  MVPN_COUNT_CTOR(LinuxAppListProvider);
}

LinuxAppListProvider::~LinuxAppListProvider() {
  MVPN_COUNT_DTOR(LinuxAppListProvider);
}

void LinuxAppListProvider::fetchEntries(const QString& dataDir,
                                        QMap<QString, QString>& map) {
  logger.debug() << "Fetch Application list from" << dataDir;

  QDirIterator iter(dataDir, QStringList() << "*.desktop", QDir::Files);
  while (iter.hasNext()) {
    QFileInfo fileinfo(iter.next());
    QSettings entry(fileinfo.filePath(), QSettings::IniFormat);
    entry.beginGroup("Desktop Entry");

    /* Filter out everything except visible applications. */
    if (entry.value("Type").toString() != "Application") {
      continue;
    }
    if (entry.value("NoDisplay", QVariant(false)).toBool()) {
      continue;
    }

    map[fileinfo.absoluteFilePath()] = entry.value("Name").toString();
  }
}

void LinuxAppListProvider::getApplicationList() {
  logger.debug() << "Fetch Application list from Linux desktop";
  QMap<QString, QString> out;

  QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
  if (pe.contains("XDG_DATA_DIRS")) {
    QStringList parts = pe.value("XDG_DATA_DIRS").split(":");
    for (const QString& part : parts) {
      fetchEntries(part.trimmed() + "/applications", out);
    }
  } else {
    fetchEntries(DESKTOP_ENTRY_LOCATION, out);
  }

  if (pe.contains("HOME")) {
    fetchEntries(pe.value("HOME") + "/.local/share/applications", out);
  }

  emit newAppList(out);
}
