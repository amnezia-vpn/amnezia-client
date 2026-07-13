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
 public:
  MacOSDaemon();
  ~MacOSDaemon();

  static MacOSDaemon* instance();

 protected:
  DnsUtils* dnsutils() override { return m_dnsutils; }
  bool supportIPUtils() const override { return true; }
  IPUtils* iputils() override { return m_iputils; }

  WireguardUtils* createWgUtils() override {
    return new WireguardUtilsMacos(this);
  }

 private:
  DnsUtilsMacos* m_dnsutils = nullptr;
  IPUtilsMacos* m_iputils = nullptr;
};

#endif  // MACOSDAEMON_H
