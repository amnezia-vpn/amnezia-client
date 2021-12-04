/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WIREGUARDUTILSMACOS_H
#define WIREGUARDUTILSMACOS_H

#include "daemon/wireguardutils.h"
#include "macosroutemonitor.h"

#include <QObject>
#include <QProcess>

class WireguardUtilsMacos final : public WireguardUtils {
  Q_OBJECT

 public:
  WireguardUtilsMacos(QObject* parent);
  ~WireguardUtilsMacos();

  bool interfaceExists() override {
    return m_tunnel.state() == QProcess::Running;
  }
  QString interfaceName() override { return m_ifname; }
  bool addInterface(const InterfaceConfig& config) override;
  bool deleteInterface() override;

  bool updatePeer(const InterfaceConfig& config) override;
  bool deletePeer(const QString& pubkey) override;
  peerStatus getPeerStatus(const QString& pubkey) override;

  bool updateRoutePrefix(const IPAddressRange& prefix, int hopindex) override;
  bool deleteRoutePrefix(const IPAddressRange& prefix, int hopindex) override;

 signals:
  void backendFailure();

 private slots:
  void tunnelStdoutReady();
  void tunnelErrorOccurred(QProcess::ProcessError error);

 private:
  QString uapiCommand(const QString& command);
  static int uapiErrno(const QString& command);
  QString waitForTunnelName(const QString& filename);

  QString m_ifname;
  QProcess m_tunnel;
  MacosRouteMonitor* m_rtmonitor = nullptr;
};

#endif  // WIREGUARDUTILSMACOS_H
