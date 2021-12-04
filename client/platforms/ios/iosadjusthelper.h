/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IOSADJUSTHELPER_H
#define IOSADJUSTHELPER_H

#include <QString>

class IOSAdjustHelper final {
 public:
  static void initialize();
  static void trackEvent(const QString& eventToken);
};

#endif  // IOSADJUSTHELPER_H
