/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IOSNETWORKWATCHER_H
#define IOSNETWORKWATCHER_H

#include <Network/Network.h>

#include "networkwatcherimpl.h"

class IOSNetworkWatcher : public NetworkWatcherImpl {
 public:
  explicit IOSNetworkWatcher(QObject* parent);
  ~IOSNetworkWatcher();

  void initialize() override;

 private:
  NetworkWatcherImpl::TransportType toTransportType(nw_path_t path);
  void controllerStateChanged();

  NetworkWatcherImpl::TransportType m_currentDefaultTransport =
      NetworkWatcherImpl::TransportType_Unknown;
  NetworkWatcherImpl::TransportType m_currentVPNTransport =
      NetworkWatcherImpl::TransportType_Unknown;
  nw_path_monitor_t m_networkMonitor = nil;
  nw_connection_t m_observableConnection = nil;
};

#endif  // IOSNETWORKWATCHER_H
