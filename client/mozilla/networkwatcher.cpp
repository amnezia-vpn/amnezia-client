/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "networkwatcher.h"

#include <QMetaEnum>

#include "leakdetector.h"
#include "logger.h"
#include "networkwatcherimpl.h"
#include "platforms/dummy/dummynetworkwatcher.h"

#ifdef MZ_WINDOWS
//#  include "platforms/windows/windowsnetworkwatcher.h"
#endif

#ifdef MZ_LINUX
//#  include "platforms/linux/linuxnetworkwatcher.h"
#endif

#ifdef MZ_MACOS
#  include "platforms/macos/macosnetworkwatcher.h"
#endif

#ifdef MZ_WASM
#  include "platforms/wasm/wasmnetworkwatcher.h"
#endif
#ifdef MZ_ANDROID
#  include "platforms/android/androidnetworkwatcher.h"
#endif

#ifdef MZ_IOS
#  include "platforms/ios/iosnetworkwatcher.h"
#endif

// How often we notify the same unsecured network
#ifndef UNIT_TEST
constexpr uint32_t NETWORK_WATCHER_TIMER_MSEC = 20000;
#endif

namespace {
Logger logger("NetworkWatcher");
}

NetworkWatcher::NetworkWatcher() { MZ_COUNT_CTOR(NetworkWatcher); }

NetworkWatcher::~NetworkWatcher() { MZ_COUNT_DTOR(NetworkWatcher); }

void NetworkWatcher::initialize() {
  logger.debug() << "Initialize";

#if defined(MZ_WINDOWS)
  //m_impl = new WindowsNetworkWatcher(this);
#elif defined(MZ_LINUX)
  //m_impl = new LinuxNetworkWatcher(this);
#elif defined(MZ_MACOS)
  m_impl = new MacOSNetworkWatcher(this);
#elif defined(MZ_WASM)
  m_impl = new WasmNetworkWatcher(this);
#elif defined(MZ_ANDROID)
  m_impl = new AndroidNetworkWatcher(this);
#elif defined(MZ_IOS)
  m_impl = new IOSNetworkWatcher(this);
#else
  m_impl = new DummyNetworkWatcher(this);
#endif

  connect(m_impl, &NetworkWatcherImpl::unsecuredNetwork, this,
          &NetworkWatcher::unsecuredNetwork);
  connect(m_impl, &NetworkWatcherImpl::networkChanged, this,
          &NetworkWatcher::networkChange);

  m_impl->initialize();

  //TODO IMPL FOR AMNEZIA
}

void NetworkWatcher::settingsChanged() {
  //TODO IMPL FOR AMNEZIA

  if (m_active) {
    logger.debug()
        << "Starting Network Watcher; Reporting of Unsecured Networks: "
        << m_reportUnsecuredNetwork;
    m_impl->start();
  } else {
    logger.debug() << "Stopping Network Watcher";
    m_impl->stop();
  }
}

void NetworkWatcher::unsecuredNetwork(const QString& networkName,
                                      const QString& networkId) {
  logger.debug() << "Unsecured network:" << logger.sensitive(networkName)
                 << "id:" << logger.sensitive(networkId);

  //TODO IMPL FOR AMNEZIA
}

QString NetworkWatcher::getCurrentTransport() {
  auto type = m_impl->getTransportType();
  QMetaEnum metaEnum = QMetaEnum::fromType<NetworkWatcherImpl::TransportType>();
  return QString(metaEnum.valueToKey(type))
      .remove("TransportType_", Qt::CaseSensitive);
}
