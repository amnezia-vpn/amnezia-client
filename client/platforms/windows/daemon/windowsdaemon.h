/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSDAEMON_H
#define WINDOWSDAEMON_H

#include <qpointer.h>

#include "daemon/daemon.h"
#include "dnsutilswindows.h"
#include "windowsfirewall.h"
#include "windowssplittunnel.h"
#include "windowstunnelservice.h"
#include "wireguardutilswindows.h"

#define TUNNEL_SERVICE_NAME L"AmneziaWGTunnel$AmneziaVPN"

class WindowsDaemon final : public Daemon {
  Q_DISABLE_COPY_MOVE(WindowsDaemon)

 public:
  WindowsDaemon();
  ~WindowsDaemon();

  void prepareActivation(const InterfaceConfig& config, int inetAdapterIndex = 0) override;
  void activateSplitTunnel(const InterfaceConfig& config, int vpnAdapterIndex = 0) override;

 protected:
  bool run(Op op, const InterfaceConfig& config) override;
  WireguardUtils* wgutils() const override { return m_wgutils.get(); }
  DnsUtils* dnsutils() override { return m_dnsutils; }

 private:
  void monitorBackendFailure();

 private:
  enum State {
    Active,
    Inactive,
  };

  int m_inetAdapterIndex = -1;

  std::unique_ptr<WireguardUtilsWindows> m_wgutils;
  DnsUtilsWindows* m_dnsutils = nullptr;
  std::unique_ptr<WindowsSplitTunnel> m_splitTunnelManager;
  QPointer<WindowsFirewall> m_firewallManager;
};

#endif  // WINDOWSDAEMON_H
