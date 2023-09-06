/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSNETWORKWATCHER_H
#define WINDOWSNETWORKWATCHER_H

#include <windows.h>
#include <wlanapi.h>

#include "networkwatcherimpl.h"

class WindowsNetworkWatcher final : public NetworkWatcherImpl {
 public:
  WindowsNetworkWatcher(QObject* parent);
  ~WindowsNetworkWatcher();

  void initialize() override;

  NetworkWatcherImpl::TransportType getTransportType() override;

 private:
  static void wlanCallback(PWLAN_NOTIFICATION_DATA data, PVOID context);

  void processWlan(PWLAN_NOTIFICATION_DATA data);

 private:
  // The handle is set during the initialization. Windows calls processWlan()
  // to inform about network changes.
  HANDLE m_wlanHandle = nullptr;
  QString m_lastBSSID;
};

#endif  // WINDOWSNETWORKWATCHER_H
