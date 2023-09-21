/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "networkwatcher.h"

#include <QMetaEnum>

//#include "controller.h"
#include "leakdetector.h"
#include "logger.h"
//#include "mozillavpn.h"
#include "networkwatcherimpl.h"
#include "platforms/dummy/dummynetworkwatcher.h"
//#include "settingsholder.h"

#ifdef MZ_WINDOWS
#  include "platforms/windows/windowsnetworkwatcher.h"
#endif

#ifdef MZ_LINUX
#  include "platforms/linux/linuxnetworkwatcher.h"
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
  m_impl = new WindowsNetworkWatcher(this);
#elif defined(MZ_LINUX)
  m_impl = new LinuxNetworkWatcher(this);
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


// TODO: IMPL FOR AMNEZIA
#if 0
  SettingsHolder* settingsHolder = SettingsHolder::instance();
  Q_ASSERT(settingsHolder);

  m_active = settingsHolder->unsecuredNetworkAlert() ||
             settingsHolder->captivePortalAlert();
  m_reportUnsecuredNetwork = settingsHolder->unsecuredNetworkAlert();
  if (m_active) {
    m_impl->start();
  }

  connect(settingsHolder, &SettingsHolder::unsecuredNetworkAlertChanged, this,
          &NetworkWatcher::settingsChanged);
  connect(settingsHolder, &SettingsHolder::captivePortalAlertChanged, this,
          &NetworkWatcher::settingsChanged);

#endif
}

void NetworkWatcher::settingsChanged() {
// TODO: IMPL FOR AMNEZIA
#if 0
  SettingsHolder* settingsHolder = SettingsHolder::instance();
  m_active = settingsHolder->unsecuredNetworkAlert() ||
             settingsHolder->captivePortalAlert();
  m_reportUnsecuredNetwork = settingsHolder->unsecuredNetworkAlert();

  if (m_active) {
    logger.debug()
        << "Starting Network Watcher; Reporting of Unsecured Networks: "
        << m_reportUnsecuredNetwork;
    m_impl->start();
  } else {
    logger.debug() << "Stopping Network Watcher";
    m_impl->stop();
  }
#endif
}

void NetworkWatcher::unsecuredNetwork(const QString& networkName,
                                      const QString& networkId) {
  logger.debug() << "Unsecured network:" << logger.sensitive(networkName)
                 << "id:" << logger.sensitive(networkId);

#ifndef UNIT_TEST
  if (!m_reportUnsecuredNetwork) {
    logger.debug() << "Disabled. Ignoring unsecured network";
    return;
  }
// TODO: IMPL FOR AMNEZIA
#if 0
  MozillaVPN* vpn = MozillaVPN::instance();

  if (vpn->state() != App::StateMain) {
    logger.debug() << "VPN not ready. Ignoring unsecured network";
    return;
  }

  Controller::State state = vpn->controller()->state();
  if (state == Controller::StateOn || state == Controller::StateConnecting ||
      state == Controller::StateCheckSubscription ||
      state == Controller::StateSwitching ||
      state == Controller::StateSilentSwitching) {
    logger.debug() << "VPN on. Ignoring unsecured network";
    return;
  }

  if (!m_networks.contains(networkId)) {
    m_networks.insert(networkId, QElapsedTimer());
  } else if (!m_networks[networkId].hasExpired(NETWORK_WATCHER_TIMER_MSEC)) {
    logger.debug() << "Notification already shown. Ignoring unsecured network";
    return;
  }

  // Let's activate the QElapsedTimer to avoid notification loops.
  m_networks[networkId].start();

  // We don't connect the system tray handler in the CTOR because it can be too
  // early. Maybe the NotificationHandler has not been created yet. We do it at
  // the first detection of an unsecured network.
  if (m_firstNotification) {
    connect(NotificationHandler::instance(),
            &NotificationHandler::notificationClicked, this,
            &NetworkWatcher::notificationClicked);
    m_firstNotification = false;
  }

  NotificationHandler::instance()->unsecuredNetworkNotification(networkName);
#endif
#endif
}


QString NetworkWatcher::getCurrentTransport() {
  auto type = m_impl->getTransportType();
  QMetaEnum metaEnum = QMetaEnum::fromType<NetworkWatcherImpl::TransportType>();
  return QString(metaEnum.valueToKey(type))
      .remove("TransportType_", Qt::CaseSensitive);
}
