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

  bool deactivate(bool emitSignals = true) override;

 protected:
  WireguardUtils* wgutils() const override { return m_wgutils; }
  DnsUtils* dnsutils() override { return m_dnsutils; }
  bool supportIPUtils() const override { return true; }
  IPUtils* iputils() override { return m_iputils; }

  WireguardUtils* createWgUtils() override {
    return new WireguardUtilsLinux(this);
  }

  void replaceActiveWgUtils(WireguardUtils* newUtils) override {
    delete m_wgutils;
    m_wgutils = static_cast<WireguardUtilsLinux*>(newUtils);
  }

 private:
  WireguardUtilsLinux* m_wgutils = nullptr;
  DnsUtilsLinux* m_dnsutils = nullptr;
  IPUtilsLinux* m_iputils = nullptr;
};

#endif  // LINUXDAEMON_H
