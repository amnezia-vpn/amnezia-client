/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IOSUTILS_H
#define IOSUTILS_H

#include <QString>

class IOSUtils final {
 public:
  static QString computerName();

  static QString IAPReceipt();
};

#endif  // IOSUTILS_H
