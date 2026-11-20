/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXDAEMON_H
#define LINUXDAEMON_H

#include "daemon/daemon.h"
#include "dnsutilslinux.h"
#include "iputilslinux.h"
#include "wireguardutilslinux.h"

class LinuxDaemon final : public Daemon {
  friend class IPUtilsMacos;

 public:
  LinuxDaemon();
  ~LinuxDaemon();

  static LinuxDaemon* instance();

 protected:
  WireguardUtils* wgutils() const override { return m_wgutils; }
  DnsUtils* dnsutils() override { return m_dnsutils; }
  bool supportIPUtils() const override { return true; }
  IPUtils* iputils() override { return m_iputils; }

  bool allowBypassAddress(const IPAddress& prefix) override {
    return m_wgutils ? m_wgutils->allowBypassTraffic(prefix) : true;
  }
  bool revokeBypassAddress(const IPAddress& prefix) override {
    return m_wgutils ? m_wgutils->revokeBypassTraffic(prefix) : true;
  }
  bool revokeAllBypassAddresses() override {
    return m_wgutils ? m_wgutils->revokeAllBypassTraffic() : true;
  }
  void reassertBypassPermits(const QList<IPAddress>& addresses,
                             const QHostAddress& resolver) override {
    if (m_wgutils) m_wgutils->reassertBypassTraffic(addresses, resolver);
  }
  // The route monitor builds IPv4 exclusion routes only (its gateway lookup
  // and RTA_GATEWAY are IPv4), so learned IPv6 addresses stay in the tunnel.
  bool supportsIPv6ExclusionRoutes() const override { return false; }
  bool allowResolverEndpoint(const QHostAddress& address) override {
    return m_wgutils ? m_wgutils->allowResolverEndpoint(address) : true;
  }
  bool revokeResolverEndpoint(const QHostAddress& address) override {
    return m_wgutils ? m_wgutils->revokeResolverEndpoint(address) : true;
  }

 private:
  WireguardUtilsLinux* m_wgutils = nullptr;
  DnsUtilsLinux* m_dnsutils = nullptr;
  IPUtilsLinux* m_iputils = nullptr;
};

#endif  // LINUXDAEMON_H
