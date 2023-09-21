/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsdaemon.h"

#include <Windows.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QNetworkInterface>
#include <QTextStream>
#include <QtGlobal>

#include "dnsutilswindows.h"
#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/windowscommons.h"
#include "platforms/windows/windowsservicemanager.h"
#include "windowsfirewall.h"

namespace {
Logger logger("WindowsDaemon");
}

WindowsDaemon::WindowsDaemon() : Daemon(nullptr), m_splitTunnelManager(this) {
  MZ_COUNT_CTOR(WindowsDaemon);

  m_wgutils = new WireguardUtilsWindows(this);
  m_dnsutils = new DnsUtilsWindows(this);

  connect(m_wgutils, &WireguardUtilsWindows::backendFailure, this,
          &WindowsDaemon::monitorBackendFailure);
  connect(this, &WindowsDaemon::activationFailure,
          []() { WindowsFirewall::instance()->disableKillSwitch(); });
}

WindowsDaemon::~WindowsDaemon() {
  MZ_COUNT_DTOR(WindowsDaemon);
  logger.debug() << "Daemon released";
}

void WindowsDaemon::prepareActivation(const InterfaceConfig& config) {
  // Before creating the interface we need to check which adapter
  // routes to the server endpoint
  auto serveraddr = QHostAddress(config.m_serverIpv4AddrIn);
  m_inetAdapterIndex = WindowsCommons::AdapterIndexTo(serveraddr);
}

bool WindowsDaemon::run(Op op, const InterfaceConfig& config) {
  if (op == Down) {
    m_splitTunnelManager.stop();
    return true;
  }

  if (op == Up) {
    logger.debug() << "Tunnel UP, Starting SplitTunneling";
    if (!WindowsSplitTunnel::isInstalled()) {
      logger.warning() << "Split Tunnel Driver not Installed yet, fixing this.";
      WindowsSplitTunnel::installDriver();
    }
  }

  if (config.m_vpnDisabledApps.length() > 0) {
    m_splitTunnelManager.start(m_inetAdapterIndex);
    m_splitTunnelManager.setRules(config.m_vpnDisabledApps);
  } else {
    m_splitTunnelManager.stop();
  }
  return true;
}

void WindowsDaemon::monitorBackendFailure() {
  logger.warning() << "Tunnel service is down";

  emit backendFailure();
  deactivate();
}
