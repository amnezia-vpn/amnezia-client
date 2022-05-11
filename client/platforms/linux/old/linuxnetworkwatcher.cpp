/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxnetworkwatcher.h"
#include "linuxnetworkwatcherworker.h"
#include "leakdetector.h"
#include "logger.h"
#include "timersingleshot.h"

namespace {
Logger logger(LOG_LINUX, "LinuxNetworkWatcher");
}

LinuxNetworkWatcher::LinuxNetworkWatcher(QObject* parent)
    : NetworkWatcherImpl(parent) {
  MVPN_COUNT_CTOR(LinuxNetworkWatcher);

  m_thread.start();
}

LinuxNetworkWatcher::~LinuxNetworkWatcher() {
  MVPN_COUNT_DTOR(LinuxNetworkWatcher);

  delete m_worker;

  m_thread.quit();
  m_thread.wait();
}

void LinuxNetworkWatcher::initialize() {
  logger.debug() << "initialize";

  m_worker = new LinuxNetworkWatcherWorker(&m_thread);

  connect(this, &LinuxNetworkWatcher::checkDevicesInThread, m_worker,
          &LinuxNetworkWatcherWorker::checkDevices);

  connect(m_worker, &LinuxNetworkWatcherWorker::unsecuredNetwork, this,
          &LinuxNetworkWatcher::unsecuredNetwork);

  // Let's wait a few seconds to allow the UI to be fully loaded and shown.
  // This is not strictly needed, but it's better for user experience because
  // it makes the UI faster to appear, plus it gives a bit of delay between the
  // UI to appear and the first notification.
  TimerSingleShot::create(this, 2000, [this]() {
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection);
  });
}

void LinuxNetworkWatcher::start() {
  logger.debug() << "actived";
  NetworkWatcherImpl::start();
  emit checkDevicesInThread();
}
