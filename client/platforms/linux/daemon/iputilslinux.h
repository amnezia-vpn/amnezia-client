/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IPUTILSLINUX_H
#define IPUTILSLINUX_H

#include <arpa/inet.h>

#include "daemon/iputils.h"

class IPUtilsLinux final : public IPUtils {
 public:
  IPUtilsLinux(QObject* parent);
  ~IPUtilsLinux();
  bool addInterfaceIPs(const InterfaceConfig& config) override;
  bool setMTUAndUp(const InterfaceConfig& config) override;

 private:
  bool addIP4AddressToDevice(const InterfaceConfig& config);
  bool addIP6AddressToDevice(const InterfaceConfig& config);

 private:
  struct in6_ifreq {
    struct in6_addr addr;
    uint32_t prefixlen;
    unsigned int ifindex;
  };
};

#endif  // IPUTILSLINUX_H