/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSDAEMONTUNNEL_H
#define WINDOWSDAEMONTUNNEL_H

#include <QObject>

class WindowsDaemonTunnel {
 public:
  explicit WindowsDaemonTunnel();
  ~WindowsDaemonTunnel();

  int run(QStringList& tokens);
};

#endif  // WINDOWSDAEMONTUNNEL_H
