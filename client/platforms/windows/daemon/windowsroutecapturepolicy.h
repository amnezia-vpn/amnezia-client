/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSROUTECAPTUREPOLICY_H
#define WINDOWSROUTECAPTUREPOLICY_H

#include <winsock2.h>
#include <WS2tcpip.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>

#include <cstring>

namespace WindowsRouteCapturePolicy {

inline bool isVpnInterfaceRoute(const MIB_IPFORWARD_ROW2* row,
                                unsigned long long vpnLuid) {
  return row->InterfaceLuid.Value == vpnLuid;
}

inline bool isDefaultRoute(const MIB_IPFORWARD_ROW2* row) {
  return row->DestinationPrefix.PrefixLength == 0;
}

inline bool isRouteCreatedByMonitor(const MIB_IPFORWARD_ROW2* row,
                                    ULONG monitorMetric) {
  return row->Protocol == MIB_IPPROTO_NETMGMT && row->Metric == monitorMetric;
}

inline bool isIpv6Unspecified(const IN6_ADDR& address) {
  static const IN6_ADDR unspecified = {};
  return std::memcmp(&address, &unspecified, sizeof(unspecified)) == 0;
}

inline bool isOnLinkRoute(const MIB_IPFORWARD_ROW2* row) {
  // Connected routes have no gateway. Capturing them into the VPN tunnel
  // breaks LAN, Hyper-V, WSL, and other locally attached networks.
  if (row->NextHop.si_family == AF_UNSPEC) {
    return true;
  }

  if (row->NextHop.si_family == AF_INET) {
    return row->NextHop.Ipv4.sin_addr.s_addr == 0;
  }

  if (row->NextHop.si_family == AF_INET6) {
    return isIpv6Unspecified(row->NextHop.Ipv6.sin6_addr);
  }

  return false;
}

inline bool shouldCaptureRoute(const MIB_IPFORWARD_ROW2* row,
                               unsigned long long vpnLuid,
                               bool routeExcluded, ULONG monitorMetric) {
  if (isVpnInterfaceRoute(row, vpnLuid)) {
    return false;
  }
  if (isDefaultRoute(row)) {
    return false;
  }
  if (isRouteCreatedByMonitor(row, monitorMetric)) {
    return false;
  }
  if (routeExcluded) {
    return false;
  }
  // Default-route capture should only clone routed prefixes. Directly
  // connected prefixes must stay bound to their real local interfaces.
  if (isOnLinkRoute(row)) {
    return false;
  }

  return true;
}

}  // namespace WindowsRouteCapturePolicy

#endif  // WINDOWSROUTECAPTUREPOLICY_H
