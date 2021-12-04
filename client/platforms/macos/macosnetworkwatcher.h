/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSNETWORKWATCHER_H
#define MACOSNETWORKWATCHER_H

#include "networkwatcherimpl.h"

class MacOSNetworkWatcher final : public NetworkWatcherImpl {
 public:
  MacOSNetworkWatcher(QObject* parent);
  ~MacOSNetworkWatcher();

  void initialize() override;

  void start() override;

  void checkInterface();

 private:
  void* m_delegate = nullptr;
};

#endif  // MACOSNETWORKWATCHER_H
