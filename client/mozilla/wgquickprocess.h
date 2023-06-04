/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WGQUICKPROCESS_H
#define WGQUICKPROCESS_H

#include <QObject>

#include "daemon/daemon.h"
#include "daemon/interfaceconfig.h"

class WgQuickProcess final {
  Q_DISABLE_COPY_MOVE(WgQuickProcess)

 public:
  static QString createConfigString(
      const InterfaceConfig& config,
      const QMap<QString, QString>& extra = QMap<QString, QString>());
};

#endif  // WGQUICKPROCESS_H
