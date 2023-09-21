/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXNETWORKWATCHER_H
#define LINUXNETWORKWATCHER_H

#include <QThread>

#include "networkwatcherimpl.h"

class LinuxNetworkWatcherWorker;

class LinuxNetworkWatcher final : public NetworkWatcherImpl {
  Q_OBJECT

 public:
  explicit LinuxNetworkWatcher(QObject* parent);
  ~LinuxNetworkWatcher();

  void initialize() override;

  void start() override;

  NetworkWatcherImpl::TransportType getTransportType() {
    // TODO: Find out how to do that on linux generally. (VPN-2382)
    return NetworkWatcherImpl::TransportType_Unknown;
  };

 signals:
  void checkDevicesInThread();

 private:
  LinuxNetworkWatcherWorker* m_worker = nullptr;
  QThread m_thread;
};

#endif  // LINUXNETWORKWATCHER_H
