/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsdaemon.h"

#include <winsock2.h>
#include <Windows.h>
#include <qassert.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QNetworkInterface>
#include <QTextStream>
#include <QtGlobal>

#include "daemon/daemonerrors.h"
#include "dnsutilswindows.h"
#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/daemon/windowsfirewall.h"
#include "platforms/windows/daemon/windowssplittunnel.h"
#include "windowsfirewall.h"

#include "core/networkUtilities.h"

namespace {
Logger logger("WindowsDaemon");
}

WindowsDaemon::WindowsDaemon() : Daemon(nullptr) {
  MZ_COUNT_CTOR(WindowsDaemon);
  m_firewallManager = WindowsFirewall::create(this);
  if (m_firewallManager == nullptr) {
    logger.error() << "Failed to create WindowsFirewall manager. Make sure you are running as Administrator.";
  }

  m_wgutils = WireguardUtilsWindows::create(m_firewallManager, this);
  if (m_wgutils == nullptr) {
    logger.error() << "Failed to create WireguardUtilsWindows. Check firewall permissions.";
  }

  m_dnsutils = new DnsUtilsWindows(this);
  m_splitTunnelManager = WindowsSplitTunnel::create(m_firewallManager);

  connect(m_wgutils.get(), &WireguardUtilsWindows::backendFailure, this,
          &WindowsDaemon::monitorBackendFailure);
  connect(this, &WindowsDaemon::activationFailure,
          [this]() { m_firewallManager->disableKillSwitch(); });
}

WindowsDaemon::~WindowsDaemon() {
  MZ_COUNT_DTOR(WindowsDaemon);
  logger.debug() << "Daemon released";
}

void WindowsDaemon::prepareActivation(const InterfaceConfig& config, int inetAdapterIndex) {
  // Before creating the interface we need to check which adapter
  // routes to the server endpoint
  if (inetAdapterIndex == 0) {
      QHostAddress serveraddr;
      if (!config.m_serverIpv4AddrIn.isEmpty()) {
          serveraddr = QHostAddress(config.m_serverIpv4AddrIn);
      } else if (!config.m_serverIpv6AddrIn.isEmpty()) {
          serveraddr = QHostAddress(config.m_serverIpv6AddrIn);
      }

      if (serveraddr.isNull()) {
          logger.warning() << "Unable to determine endpoint address for adapter selection";
          m_inetAdapterIndex = -1;
          return;
      }

      m_inetAdapterIndex = NetworkUtilities::AdapterIndexTo(serveraddr);
  } else {
      m_inetAdapterIndex = inetAdapterIndex;
  }
}

void WindowsDaemon::activateSplitTunnel(const InterfaceConfig& config, int vpnAdapterIndex) {
    if (m_splitTunnelManager == nullptr)
        return;

  if (config.m_vpnDisabledApps.length() > 0) {
      m_splitTunnelManager->start(m_inetAdapterIndex, vpnAdapterIndex);
      m_splitTunnelManager->excludeApps(config.m_vpnDisabledApps);
  } else {
      m_splitTunnelManager->stop();
  }
}

bool WindowsDaemon::run(Op op, const InterfaceConfig& config) {
  if (!m_splitTunnelManager) {
    if (config.m_vpnDisabledApps.length() > 0) {
      // The Client has sent us a list of disabled apps, but we failed
      // to init the the split tunnel driver.
      // So let the client know this was not possible
      emit backendFailure(DaemonError::ERROR_SPLIT_TUNNEL_INIT_FAILURE);
    }
    return true;
  }

  if (op == Down) {
    m_splitTunnelManager->stop();
    return true;
  }
  if (config.m_vpnDisabledApps.length() > 0) {
    if (m_inetAdapterIndex <= 0) {
      emit backendFailure(DaemonError::ERROR_SPLIT_TUNNEL_START_FAILURE);
      m_splitTunnelManager->stop();
      return true;
    }

    if (!m_splitTunnelManager->start(m_inetAdapterIndex)) {
      emit backendFailure(DaemonError::ERROR_SPLIT_TUNNEL_START_FAILURE);
      m_splitTunnelManager->stop();
      return true;
    }
    if (!m_splitTunnelManager->excludeApps(config.m_vpnDisabledApps)) {
      emit backendFailure(DaemonError::ERROR_SPLIT_TUNNEL_EXCLUDE_FAILURE);
      m_splitTunnelManager->stop();
      return true;
    }
    // Now the driver should be running (State == 4)
    if (!m_splitTunnelManager->isRunning()) {
      emit backendFailure(DaemonError::ERROR_SPLIT_TUNNEL_START_FAILURE);
      m_splitTunnelManager->stop();
      return true;
    }
    return true;
  }
  m_splitTunnelManager->stop();

  return true;
}

void WindowsDaemon::monitorBackendFailure() {
  logger.warning() << "Tunnel service is down";

  emit backendFailure();
  deactivate();
}
