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

#include <QByteArray>
#include <QHash>
#include <QSet>

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

inline bool isConnectedLocalRoute(const MIB_IPFORWARD_ROW2* row,
                                  unsigned long long vpnLuid) {
  if (isVpnInterfaceRoute(row, vpnLuid) || isDefaultRoute(row) ||
      row->Loopback) {
    return false;
  }

  // MIB_IPPROTO_LOCAL identifies routes added locally on an interface. Using
  // it here avoids treating arbitrary NETMGMT/static on-link routes as a
  // connected network that could exempt a wider address range from capture.
  return row->Protocol == MIB_IPPROTO_LOCAL && isOnLinkRoute(row);
}

class ConnectedPrefixIndex final {
 public:
  void add(const IP_ADDRESS_PREFIX* prefix) {
    const int prefixLength = prefix->PrefixLength;
    if (prefix->Prefix.si_family == AF_INET) {
      if (prefixLength < 0 || prefixLength > 32) {
        return;
      }
      m_ipv4[prefixLength].insert(
          ipv4Network(prefix->Prefix.Ipv4.sin_addr, prefixLength));
      return;
    }

    if (prefix->Prefix.si_family == AF_INET6) {
      if (prefixLength < 0 || prefixLength > 128) {
        return;
      }
      m_ipv6[prefixLength].insert(
          ipv6Network(prefix->Prefix.Ipv6.sin6_addr, prefixLength));
    }
  }

  bool contains(const IP_ADDRESS_PREFIX* destination) const {
    const int destinationLength = destination->PrefixLength;
    if (destination->Prefix.si_family == AF_INET) {
      if (destinationLength < 0 || destinationLength > 32) {
        return false;
      }

      for (int prefixLength = destinationLength; prefixLength >= 0;
           --prefixLength) {
        auto i = m_ipv4.constFind(prefixLength);
        if (i == m_ipv4.constEnd()) {
          continue;
        }
        if (i.value().contains(ipv4Network(
                destination->Prefix.Ipv4.sin_addr, prefixLength))) {
          return true;
        }
      }
      return false;
    }

    if (destination->Prefix.si_family == AF_INET6) {
      if (destinationLength < 0 || destinationLength > 128) {
        return false;
      }

      for (int prefixLength = destinationLength; prefixLength >= 0;
           --prefixLength) {
        auto i = m_ipv6.constFind(prefixLength);
        if (i == m_ipv6.constEnd()) {
          continue;
        }
        if (i.value().contains(ipv6Network(
                destination->Prefix.Ipv6.sin6_addr, prefixLength))) {
          return true;
        }
      }
    }

    return false;
  }

 private:
  static quint32 ipv4Network(const IN_ADDR& address, int prefixLength) {
    quint32 network = ntohl(address.s_addr);
    if (prefixLength <= 0) {
      return 0;
    }
    if (prefixLength >= 32) {
      return network;
    }
    return network & (0xffffffffu << (32 - prefixLength));
  }

  static QByteArray ipv6Network(const IN6_ADDR& address, int prefixLength) {
    QByteArray network(reinterpret_cast<const char*>(&address), sizeof(address));
    if (prefixLength <= 0) {
      network.fill('\0');
      return network;
    }
    if (prefixLength >= 128) {
      return network;
    }

    const int fullBytes = prefixLength / 8;
    const int partialBits = prefixLength % 8;
    if (partialBits != 0) {
      const auto value = static_cast<unsigned char>(network.at(fullBytes));
      const auto mask = static_cast<unsigned char>(0xffu << (8 - partialBits));
      network[fullBytes] = static_cast<char>(value & mask);
    }

    const int zeroFrom = fullBytes + (partialBits != 0 ? 1 : 0);
    if (zeroFrom < network.size()) {
      std::memset(network.data() + zeroFrom, 0, network.size() - zeroFrom);
    }
    return network;
  }

  QHash<int, QSet<quint32>> m_ipv4;
  QHash<int, QSet<QByteArray>> m_ipv6;
};

inline bool shouldCaptureRoute(const MIB_IPFORWARD_ROW2* row,
                               unsigned long long vpnLuid,
                               bool routeExcluded,
                               bool destinationOnConnectedNetwork,
                               ULONG monitorMetric) {
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
  // A static or routed host prefix can still point through a LAN gateway. If
  // its entire destination lies inside a real connected prefix, keep it on the
  // physical/local interface instead of cloning it into the VPN.
  if (destinationOnConnectedNetwork) {
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
