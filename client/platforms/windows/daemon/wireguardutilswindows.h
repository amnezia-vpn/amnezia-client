/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WIREGUARDUTILSWINDOWS_H
#define WIREGUARDUTILSWINDOWS_H

#include <windows.h>

#include <QHostAddress>
#include <QObject>

#include "daemon/wireguardutils.h"
#include "windowsroutemonitor.h"
#include "windowstunnelservice.h"

class WireguardUtilsWindows final : public WireguardUtils {
  Q_OBJECT

 public:
  WireguardUtilsWindows(QObject* parent);
  ~WireguardUtilsWindows();

  bool interfaceExists() override { return m_tunnel.isRunning(); }
  QString interfaceName() override {
    return WireguardUtilsWindows::s_interfaceName();
  }
  static const QString s_interfaceName() { return "MozillaVPN"; }
  bool addInterface(const InterfaceConfig& config) override;
  bool deleteInterface() override;

  bool updatePeer(const InterfaceConfig& config) override;
  bool deletePeer(const InterfaceConfig& config) override;
  QList<PeerStatus> getPeerStatus() override;

  bool updateRoutePrefix(const IPAddress& prefix) override;
  bool deleteRoutePrefix(const IPAddress& prefix) override;

  bool addExclusionRoute(const IPAddress& prefix) override;
  bool deleteExclusionRoute(const IPAddress& prefix) override;

 signals:
  void backendFailure();

 private:
  void buildMibForwardRow(const IPAddress& prefix, void* row);

  quint64 m_luid = 0;
  WindowsTunnelService m_tunnel;
  WindowsRouteMonitor m_routeMonitor;
};

#endif  // WIREGUARDUTILSWINDOWS_H
