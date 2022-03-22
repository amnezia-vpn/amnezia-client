/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef POLKITHELPER_H
#define POLKITHELPER_H

#include <QString>

class PolkitHelper final {
 public:
  static PolkitHelper* instance();

  bool checkAuthorization(const QString& actionId);

 private:
  PolkitHelper() = default;
  ~PolkitHelper() = default;

  Q_DISABLE_COPY(PolkitHelper)
};

#endif  // POLKITHELPER_H
