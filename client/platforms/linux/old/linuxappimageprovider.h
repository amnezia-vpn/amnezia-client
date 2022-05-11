/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXAPPIMAGEPROVIDER_H
#define LINUXAPPIMAGEPROVIDER_H

#include "appimageprovider.h"

class LinuxAppImageProvider final : public AppImageProvider {
 public:
  LinuxAppImageProvider(QObject* parent);
  ~LinuxAppImageProvider();
  QImage requestImage(const QString& id, QSize* size,
                      const QSize& requestedSize) override;

 private:
  static void addFallbackPaths(const QString& dataDir,
                               QStringList& fallbackPaths);
};

#endif  // LINUXAPPIMAGEPROVIDER_H
