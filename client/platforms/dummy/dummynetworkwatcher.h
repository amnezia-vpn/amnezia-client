/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DUMMYNETWORKWATCHER_H
#define DUMMYNETWORKWATCHER_H

#include "networkwatcherimpl.h"

class DummyNetworkWatcher final : public NetworkWatcherImpl {
 public:
  DummyNetworkWatcher(QObject* parent);
  ~DummyNetworkWatcher();

  void initialize() override;

  NetworkWatcherImpl::TransportType getTransportType() override {
    return TransportType_Other;
  };
};

#endif  // DUMMYNETWORKWATCHER_H
