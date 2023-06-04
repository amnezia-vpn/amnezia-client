/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iosnetworkwatcher.h"

#include "leakdetector.h"
#include "logger.h"

#import <Network/Network.h>

namespace {
Logger logger("IOSNetworkWatcher");
dispatch_queue_t s_queue = dispatch_queue_create("VPNNetwork.queue", DISPATCH_QUEUE_SERIAL);
}

IOSNetworkWatcher::IOSNetworkWatcher(QObject* parent) : NetworkWatcherImpl(parent) {
  MZ_COUNT_CTOR(IOSNetworkWatcher);
}

IOSNetworkWatcher::~IOSNetworkWatcher() {
  MZ_COUNT_DTOR(IOSNetworkWatcher);
  if (m_networkMonitor != nil) {
    nw_path_monitor_cancel(m_networkMonitor);
    nw_release(m_networkMonitor);
  }
}

void IOSNetworkWatcher::initialize() {
  m_networkMonitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(m_networkMonitor, s_queue);
  nw_path_monitor_set_update_handler(m_networkMonitor, ^(nw_path_t _Nonnull path) {
    m_currentDefaultTransport = toTransportType(path);
  });
  nw_path_monitor_start(m_networkMonitor);

  //TODO IMPL FOR AMNEZIA
}

NetworkWatcherImpl::TransportType IOSNetworkWatcher::getTransportType() {
  //TODO IMPL FOR AMNEZIA

  if (m_observableConnection != nil) {
    return m_currentVPNTransport;
  }
  // If we don't have an open tunnel-observer, m_currentVPNTransport is probably wrong.
  return NetworkWatcherImpl::TransportType_Unknown;
}

NetworkWatcherImpl::TransportType IOSNetworkWatcher::toTransportType(nw_path_t path) {
  if (path == nil) {
    return NetworkWatcherImpl::TransportType_Unknown;
  }
  auto status = nw_path_get_status(path);
  if (status != nw_path_status_satisfied && status != nw_path_status_satisfiable) {
    // We're offline.
    return NetworkWatcherImpl::TransportType_None;
  }
  if (nw_path_uses_interface_type(path, nw_interface_type_wifi)) {
    return NetworkWatcherImpl::TransportType_WiFi;
  }
  if (nw_path_uses_interface_type(path, nw_interface_type_wired)) {
    return NetworkWatcherImpl::TransportType_Ethernet;
  }
  if (nw_path_uses_interface_type(path, nw_interface_type_cellular)) {
    return NetworkWatcherImpl::TransportType_Cellular;
  }
  if (nw_path_uses_interface_type(path, nw_interface_type_other)) {
    return NetworkWatcherImpl::TransportType_Other;
  }
  if (nw_path_uses_interface_type(path, nw_interface_type_loopback)) {
    return NetworkWatcherImpl::TransportType_Other;
  }
  return NetworkWatcherImpl::TransportType_Unknown;
}

void IOSNetworkWatcher::controllerStateChanged() {
  //TODO IMPL FOR AMNEZIA
}
