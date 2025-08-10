/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WIREGUARDUTILSWINDOWS_H
#define WIREGUARDUTILSWINDOWS_H

#include <windows.h>

#include <QHostAddress>
#include <QObject>
#include <QPointer>

#include "daemon/wireguardutils.h"
#include "windowsroutemonitor.h"
#include "windowstunnelservice.h"

class WindowsFirewall;
class WindowsRouteMonitor;

class WireguardUtilsWindows final : public WireguardUtils {
  Q_OBJECT

 public:
  static std::unique_ptr<WireguardUtilsWindows> create(WindowsFirewall* fw,
                                                       QObject* parent);
  ~WireguardUtilsWindows();

  bool interfaceExists() override { return m_tunnel.isRunning(); }
  QString interfaceName() override {
    return WireguardUtilsWindows::s_interfaceName();
  }
  static const QString s_interfaceName() { return "AmneziaVPN"; }
  bool addInterface(const InterfaceConfig& config) override;
  bool deleteInterface() override;

  bool updatePeer(const InterfaceConfig& config) override;
  bool deletePeer(const InterfaceConfig& config) override;
  QList<PeerStatus> getPeerStatus() override;

  bool updateRoutePrefix(const IPAddress& prefix) override;
  bool deleteRoutePrefix(const IPAddress& prefix) override;

  bool addExclusionRoute(const IPAddress& prefix) override;
  bool deleteExclusionRoute(const IPAddress& prefix) override;

  bool WireguardUtilsWindows::excludeLocalNetworks(const QList<IPAddress>& addresses) override;

 signals:
  void backendFailure();

 private:
  WireguardUtilsWindows(QObject* parent, WindowsFirewall* fw);
  void buildMibForwardRow(const IPAddress& prefix, void* row);

  quint64 m_luid = 0;
  WindowsTunnelService m_tunnel;
  QPointer<WindowsRouteMonitor> m_routeMonitor;
  QPointer<WindowsFirewall> m_firewall;
};

#endif  // WIREGUARDUTILSWINDOWS_H
