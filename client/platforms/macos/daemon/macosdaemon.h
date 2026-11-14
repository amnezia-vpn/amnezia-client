/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSDAEMON_H
#define MACOSDAEMON_H

#include "daemon/daemon.h"
#include "dnsutilsmacos.h"
#include "iputilsmacos.h"
#include "wireguardutilsmacos.h"

class MacOSDaemon final : public Daemon {
  friend class IPUtilsMacos;

 public:
  MacOSDaemon();
  ~MacOSDaemon();

  static MacOSDaemon* instance();

 protected:
  WireguardUtils* wgutils() const override { return m_wgutils; }

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
  bool allowResolverEndpoint(const QHostAddress& address) override {
    return m_wgutils ? m_wgutils->allowResolverEndpoint(address) : true;
  }
  bool revokeResolverEndpoint(const QHostAddress& address) override {
    return m_wgutils ? m_wgutils->revokeResolverEndpoint(address) : true;
  }
  DnsUtils* dnsutils() override { return m_dnsutils; }
  bool supportIPUtils() const override { return true; }
  IPUtils* iputils() override { return m_iputils; }

 private:
  WireguardUtilsMacos* m_wgutils = nullptr;
  DnsUtilsMacos* m_dnsutils = nullptr;
  IPUtilsMacos* m_iputils = nullptr;
};

#endif  // MACOSDAEMON_H
