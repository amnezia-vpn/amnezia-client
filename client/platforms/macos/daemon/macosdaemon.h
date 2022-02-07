/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSDAEMON_H
#define MACOSDAEMON_H

#include "daemon.h"
#include "dnsutilsmacos.h"
#include "iputilsmacos.h"
#include "wireguardutilsmacos.h"

class MacOSDaemon final : public Daemon {
  friend class IPUtilsMacos;

 public:
  MacOSDaemon();
  ~MacOSDaemon();

  static MacOSDaemon* instance();

  QByteArray getStatus() override;

 protected:
  WireguardUtils* wgutils() const override { return m_wgutils; }
  bool supportDnsUtils() const override { return true; }
  DnsUtils* dnsutils() override { return m_dnsutils; }
  bool supportIPUtils() const override { return true; }
  IPUtils* iputils() override { return m_iputils; }

 private:
  WireguardUtilsMacos* m_wgutils = nullptr;
  DnsUtilsMacos* m_dnsutils = nullptr;
  IPUtilsMacos* m_iputils = nullptr;
};

#endif  // MACOSDAEMON_H
