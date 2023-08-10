/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSNETWORKWATCHER_H
#define MACOSNETWORKWATCHER_H

#import <Network/Network.h>

#include "../ios/iosnetworkwatcher.h"
#include "networkwatcherimpl.h"

class QString;

class MacOSNetworkWatcher final : public IOSNetworkWatcher {
 public:
  MacOSNetworkWatcher(QObject* parent);
  ~MacOSNetworkWatcher();

  void start() override;

  void checkInterface();

  void controllerStateChanged();

 private:
  void* m_delegate = nullptr;
};

#endif  // MACOSNETWORKWATCHER_H
