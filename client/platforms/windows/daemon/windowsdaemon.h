/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSDAEMON_H
#define WINDOWSDAEMON_H

#include "daemon/daemon.h"
#include "dnsutilswindows.h"
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
  WireguardUtils* wgutils() const override { return m_wgutils; }
  DnsUtils* dnsutils() override { return m_dnsutils; }

 private:
  void monitorBackendFailure();

 private:
  enum State {
    Active,
    Inactive,
  };

  int m_inetAdapterIndex = -1;

  WireguardUtilsWindows* m_wgutils = nullptr;
  DnsUtilsWindows* m_dnsutils = nullptr;
  WindowsSplitTunnel m_splitTunnelManager;
};

#endif  // WINDOWSDAEMON_H
