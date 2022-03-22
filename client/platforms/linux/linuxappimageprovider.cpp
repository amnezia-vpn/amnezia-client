/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxappimageprovider.h"
#include "logger.h"
#include "leakdetector.h"

#include <QDir>
#include <QDirIterator>
#include <QProcessEnvironment>
#include <QString>
#include <QSettings>
#include <QIcon>

constexpr const char* PIXMAP_FALLBACK_PATH = "/usr/share/pixmaps/";
constexpr const char* DESKTOP_ICON_LOCATION = "/usr/share/icons/";

namespace {
Logger logger(LOG_CONTROLLER, "LinuxAppImageProvider");
}

LinuxAppImageProvider::LinuxAppImageProvider(QObject* parent)
    : AppImageProvider(parent, QQuickImageProvider::Image,
                       QQmlImageProviderBase::ForceAsynchronousImageLoading) {
  MVPN_COUNT_CTOR(LinuxAppImageProvider);

  QStringList searchPaths = QIcon::fallbackSearchPaths();

  QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
  if (pe.contains("XDG_DATA_DIRS")) {
    QStringList parts = pe.value("XDG_DATA_DIRS").split(":");
    for (const QString& part : parts) {
      addFallbackPaths(part + "/icons", searchPaths);
    }
  } else {
    addFallbackPaths(DESKTOP_ICON_LOCATION, searchPaths);
  }

  if (pe.contains("HOME")) {
    addFallbackPaths(pe.value("HOME") + "/.local/share/icons", searchPaths);
  }

  searchPaths << PIXMAP_FALLBACK_PATH;
  QIcon::setFallbackSearchPaths(searchPaths);
}

LinuxAppImageProvider::~LinuxAppImageProvider() {
  MVPN_COUNT_DTOR(LinuxAppImageProvider);
}

void LinuxAppImageProvider::addFallbackPaths(const QString& iconDir,
                                             QStringList& searchPaths) {
  searchPaths << iconDir;

  QDirIterator iter(iconDir, QDir::Dirs | QDir::NoDotAndDotDot);
  while (iter.hasNext()) {
    QFileInfo fileinfo(iter.next());
    logger.debug() << "Adding QIcon fallback:" << fileinfo.absoluteFilePath();
    searchPaths << fileinfo.absoluteFilePath();
  }
}

// from QQuickImageProvider
QImage LinuxAppImageProvider::requestImage(const QString& id, QSize* size,
                                           const QSize& requestedSize) {
  QSettings entry(id, QSettings::IniFormat);
  entry.beginGroup("Desktop Entry");
  QString name = entry.value("Icon").toString();

  QIcon icon = QIcon::fromTheme(name);
  QPixmap pixmap = icon.pixmap(requestedSize);
  size->setHeight(pixmap.height());
  size->setWidth(pixmap.width());
  logger.debug() << "Loaded icon" << icon.name() << "size:" << pixmap.width()
                 << "x" << pixmap.height();

  return pixmap.toImage();
}
