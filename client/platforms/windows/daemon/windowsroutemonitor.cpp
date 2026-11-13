/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsroutemonitor.h"

#include <QElapsedTimer>
#include <QScopeGuard>

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("WindowsRouteMonitor");
};  // namespace

// Attempt to mark routing entries that we create with a relatively
// high metric. This ensures that we can skip over routes of our own
// creation when processing route changes, and ensures that we give
// way to other routing entries.
constexpr const ULONG EXCLUSION_ROUTE_METRIC = 0x5e72;

// Called by the kernel on route changes - perform some basic filtering and
// invoke the routeChanged slot to do the real work.
static void routeChangeCallback(PVOID context, PMIB_IPFORWARD_ROW2 row,
                                MIB_NOTIFICATION_TYPE type) {
  WindowsRouteMonitor* monitor = (WindowsRouteMonitor*)context;
  Q_UNUSED(type);

  // Ignore route changes that we created.
  if ((row->Protocol == MIB_IPPROTO_NETMGMT) &&
      (row->Metric == EXCLUSION_ROUTE_METRIC)) {
    return;
  }
  if (monitor->getLuid() == row->InterfaceLuid.Value) {
    return;
  }

  // Invoke the route changed signal to do the real work in Qt.
  QMetaObject::invokeMethod(monitor, "routeChanged", Qt::QueuedConnection);
}

// Perform prefix matching comparison on IP addresses in host order.
static int prefixcmp(const void* a, const void* b, size_t bits) {
  size_t bytes = bits / 8;
  if (bytes > 0) {
    int diff = memcmp(a, b, bytes);
    if (diff != 0) {
      return diff;
    }
  }

  if (bits % 8) {
    quint8 avalue = *((const quint8*)a + bytes) >> (8 - bits % 8);
    quint8 bvalue = *((const quint8*)b + bytes) >> (8 - bits % 8);
    return avalue - bvalue;
  }

  return 0;
}

WindowsRouteMonitor::WindowsRouteMonitor(quint64 luid, QObject* parent)
    : QObject(parent), m_luid(luid) {
  MZ_COUNT_CTOR(WindowsRouteMonitor);
  logger.debug() << "WindowsRouteMonitor created.";

  NotifyRouteChange2(AF_INET, routeChangeCallback, this, FALSE, &m_routeHandle);
}

WindowsRouteMonitor::~WindowsRouteMonitor() {
  MZ_COUNT_DTOR(WindowsRouteMonitor);
  CancelMibChangeNotify2(m_routeHandle);

  flushRouteTable(m_exclusionRoutes);
  flushRouteTable(m_clonedRoutes);
  logger.debug() << "WindowsRouteMonitor destroyed.";
}

bool WindowsRouteMonitor::updateInterfaceMetrics(int family) {
  PMIB_IPINTERFACE_TABLE table;
  DWORD result = GetIpInterfaceTable(family, &table);
  if (result != NO_ERROR) {
    logger.warning() << "Failed to retrive interface table." << result;
    return false;
  }
  auto guard = qScopeGuard([&] { FreeMibTable(table); });

  // Flush the list of interfaces that are valid for routing.
  if ((family == AF_INET) || (family == AF_UNSPEC)) {
    m_interfaceMetricsIpv4.clear();
  }
  if ((family == AF_INET6) || (family == AF_UNSPEC)) {
    m_interfaceMetricsIpv6.clear();
  }

  // Rebuild the list of interfaces that are valid for routing.
  for (ULONG i = 0; i < table->NumEntries; i++) {
    MIB_IPINTERFACE_ROW* row = &table->Table[i];
    if (row->InterfaceLuid.Value == m_luid) {
      continue;
    }
    if (!row->Connected) {
      continue;
    }

    if (row->Family == AF_INET) {
      logger.debug() << "Interface" << row->InterfaceIndex
                     << "is valid for IPv4 routing";
      m_interfaceMetricsIpv4[row->InterfaceLuid.Value] = row->Metric;
    }
    if (row->Family == AF_INET6) {
      logger.debug() << "Interface" << row->InterfaceIndex
                     << "is valid for IPv6 routing";
      m_interfaceMetricsIpv6[row->InterfaceLuid.Value] = row->Metric;
    }
  }
  return true;
}

bool WindowsRouteMonitor::updateExclusionRoute(MIB_IPFORWARD_ROW2* data,
                                               void* ptable) {
  PMIB_IPFORWARD_TABLE2 table = reinterpret_cast<PMIB_IPFORWARD_TABLE2>(ptable);
  SOCKADDR_INET nexthop = {};
  quint64 bestLuid = 0;
  int bestMatch = -1;
  ULONG bestMetric = ULONG_MAX;

  nexthop.si_family = data->DestinationPrefix.Prefix.si_family;
  for (ULONG i = 0; i < table->NumEntries; i++) {
    MIB_IPFORWARD_ROW2* row = &table->Table[i];
    // Ignore routes into the VPN interface.
    if (row->InterfaceLuid.Value == m_luid) {
      continue;
    }
    if (row->DestinationPrefix.PrefixLength < bestMatch) {
      continue;
    }
    // Ignore routes of our own creation.
    if ((row->Protocol == data->Protocol) && (row->Metric == data->Metric)) {
      continue;
    }

    // Check if the routing table entry matches the destination.
    if (!routeContainsDest(&row->DestinationPrefix, &data->DestinationPrefix)) {
      continue;
    }

    // Compute the combined interface and routing metric.
    ULONG routeMetric = row->Metric;
    if (data->DestinationPrefix.Prefix.si_family == AF_INET6) {
      if (!m_interfaceMetricsIpv6.contains(row->InterfaceLuid.Value)) {
        continue;
      }
      routeMetric += m_interfaceMetricsIpv6[row->InterfaceLuid.Value];
    } else if (data->DestinationPrefix.Prefix.si_family == AF_INET) {
      if (!m_interfaceMetricsIpv4.contains(row->InterfaceLuid.Value)) {
        continue;
      }
      routeMetric += m_interfaceMetricsIpv4[row->InterfaceLuid.Value];
    } else {
      // Unsupported destination address family.
      continue;
    }
    if (routeMetric < row->Metric) {
      routeMetric = ULONG_MAX;
    }

    // Prefer routes with lower metric if we find multiple matches
    // with the same prefix length.
    if ((row->DestinationPrefix.PrefixLength == bestMatch) &&
        (routeMetric >= bestMetric)) {
      continue;
    }

    // If we got here, then this is the longest prefix match so far.
    memcpy(&nexthop, &row->NextHop, sizeof(SOCKADDR_INET));
    bestMatch = row->DestinationPrefix.PrefixLength;
    bestMetric = routeMetric;
    if (bestMatch == data->DestinationPrefix.PrefixLength) {
      bestLuid = 0;  // Don't write to the table if we find an exact match.
    } else {
      bestLuid = row->InterfaceLuid.Value;
    }
  }

  // If neither the interface nor next-hop have changed, then do nothing.
  if (data->InterfaceLuid.Value == bestLuid &&
      memcmp(&nexthop, &data->NextHop, sizeof(SOCKADDR_INET)) == 0) {
    return true;
  }

  // Delete the previous routing table entry, if any.
  if (data->InterfaceLuid.Value != 0) {
    DWORD result = DeleteIpForwardEntry2(data);
    if ((result != NO_ERROR) && (result != ERROR_NOT_FOUND)) {
      logger.error() << "Failed to delete route:" << result;
    }
  }

  // Update the routing table entry.
  data->InterfaceLuid.Value = bestLuid;
  memcpy(&data->NextHop, &nexthop, sizeof(SOCKADDR_INET));
  if (data->InterfaceLuid.Value != 0) {
    DWORD result = CreateIpForwardEntry2(data);
    if (result != NO_ERROR) {
      logger.error() << "Failed to update route:" << result;
      return false;
    }
  }
  return true;
}

// static
bool WindowsRouteMonitor::routeContainsDest(const IP_ADDRESS_PREFIX* route,
                                            const IP_ADDRESS_PREFIX* dest) {
  if (route->Prefix.si_family != dest->Prefix.si_family) {
    return false;
  }
  if (route->PrefixLength > dest->PrefixLength) {
    return false;
  }
  if (route->Prefix.si_family == AF_INET) {
    return prefixcmp(&route->Prefix.Ipv4.sin_addr, &dest->Prefix.Ipv4.sin_addr,
                     route->PrefixLength) == 0;
  } else if (route->Prefix.si_family == AF_INET6) {
    return prefixcmp(&route->Prefix.Ipv6.sin6_addr,
                     &dest->Prefix.Ipv6.sin6_addr, route->PrefixLength) == 0;
  } else {
    return false;
  }
}

// static
QHostAddress WindowsRouteMonitor::prefixToAddress(
    const IP_ADDRESS_PREFIX* dest) {
  if (dest->Prefix.si_family == AF_INET6) {
    return QHostAddress(dest->Prefix.Ipv6.sin6_addr.s6_addr);
  } else if (dest->Prefix.si_family == AF_INET) {
    quint32 addr = htonl(dest->Prefix.Ipv4.sin_addr.s_addr);
    return QHostAddress(addr);
  } else {
    return QHostAddress();
  }
}

bool WindowsRouteMonitor::isRouteExcluded(const IP_ADDRESS_PREFIX* dest) const {
  auto i = m_exclusionRoutes.constBegin();
  while (i != m_exclusionRoutes.constEnd()) {
    const MIB_IPFORWARD_ROW2* row = i.value();
    if (routeContainsDest(&row->DestinationPrefix, dest)) {
      return true;
    }
    i++;
  }
  return false;
}

void WindowsRouteMonitor::updateCapturedRoutes(int family) {
  if (!m_defaultRouteCapture) {
    return;
  }

  PMIB_IPFORWARD_TABLE2 table;
  DWORD error = GetIpForwardTable2(family, &table);
  if (error != NO_ERROR) {
    updateCapturedRoutes(family, table);
    FreeMibTable(table);
  }
}

void WindowsRouteMonitor::updateCapturedRoutes(int family, void* ptable) {
  PMIB_IPFORWARD_TABLE2 table = reinterpret_cast<PMIB_IPFORWARD_TABLE2>(ptable);
  if (!m_defaultRouteCapture) {
    return;
  }

  for (ULONG i = 0; i < table->NumEntries; i++) {
    MIB_IPFORWARD_ROW2* row = &table->Table[i];
    // Ignore routes into the VPN interface.
    if (row->InterfaceLuid.Value == m_luid) {
      continue;
    }
    // Ignore the default route
    if (row->DestinationPrefix.PrefixLength == 0) {
      continue;
    }
    // Ignore routes of our own creation.
    if ((row->Protocol == MIB_IPPROTO_NETMGMT) &&
        (row->Metric == EXCLUSION_ROUTE_METRIC)) {
      continue;
    }
    // Ignore routes which should be excluded.
    if (isRouteExcluded(&row->DestinationPrefix)) {
      continue;
    }
    QHostAddress destination = prefixToAddress(&row->DestinationPrefix);
    if (destination.isLoopback() || destination.isBroadcast() ||
        destination.isLinkLocal() || destination.isMulticast()) {
      continue;
    }

    // If we get here, this route should be cloned.
    IPAddress prefix(destination, row->DestinationPrefix.PrefixLength);
    MIB_IPFORWARD_ROW2* data = m_clonedRoutes.value(prefix, nullptr);
    if (data != nullptr) {
      // Count the number of matching entries in the main table.
      data->Age++;
      continue;
    }
    logger.debug() << "Capturing route to" << prefix.toString();

    // Clone the route and direct it into the VPN tunnel.
    data = new MIB_IPFORWARD_ROW2;
    InitializeIpForwardEntry(data);
    data->InterfaceLuid.Value = m_luid;
    data->DestinationPrefix = row->DestinationPrefix;
    data->NextHop.si_family = data->DestinationPrefix.Prefix.si_family;

    // Set the rest of the flags for a static route.
    data->ValidLifetime = 0xffffffff;
    data->PreferredLifetime = 0xffffffff;
    data->Metric = 0;
    data->Protocol = MIB_IPPROTO_NETMGMT;
    data->Loopback = false;
    data->AutoconfigureAddress = false;
    data->Publish = false;
    data->Immortal = false;
    data->Age = 0;

    // Route this traffic into the VPN tunnel.
    DWORD result = CreateIpForwardEntry2(data);
    if (result != NO_ERROR) {
      logger.error() << "Failed to update route:" << result;
      delete data;
    } else {
      m_clonedRoutes.insert(prefix, data);
      data->Age++;
    }
  }

  // Finally scan for any routes which were removed from the table. We do this
  // by reusing the age field to count the number of matching entries in the
  // main table.
  auto i = m_clonedRoutes.begin();
  while (i != m_clonedRoutes.end()) {
    MIB_IPFORWARD_ROW2* data = i.value();
    if (data->Age > 0) {
      // Entry is in use, don't delete it.
      data->Age = 0;
      i++;
      continue;
    }
    if ((family != AF_UNSPEC) &&
        (data->DestinationPrefix.Prefix.si_family != family)) {
      // We are not processing updates to this address family.
      i++;
      continue;
    }

    logger.debug() << "Removing route capture for" << i.key().toString();

    // Otherwise, this route is no longer in use.
    DWORD result = DeleteIpForwardEntry2(data);
    if ((result != NO_ERROR) && (result != ERROR_NOT_FOUND)) {
      logger.error() << "Failed to delete route:" << result;
    }
    delete data;
    i = m_clonedRoutes.erase(i);
  }
}

// static
void WindowsRouteMonitor::initExclusionRow(const IPAddress& prefix,
                                           MIB_IPFORWARD_ROW2* data) {
  InitializeIpForwardEntry(data);
  if (prefix.address().protocol() == QAbstractSocket::IPv6Protocol) {
    Q_IPV6ADDR buf = prefix.address().toIPv6Address();

    memcpy(&data->DestinationPrefix.Prefix.Ipv6.sin6_addr, &buf, sizeof(buf));
    data->DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
    data->DestinationPrefix.PrefixLength = prefix.prefixLength();
  } else {
    quint32 buf = prefix.address().toIPv4Address();

    data->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = htonl(buf);
    data->DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
    data->DestinationPrefix.PrefixLength = prefix.prefixLength();
  }
  data->NextHop.si_family = data->DestinationPrefix.Prefix.si_family;

  // Set the rest of the flags for a static route.
  data->ValidLifetime = 0xffffffff;
  data->PreferredLifetime = 0xffffffff;
  data->Metric = EXCLUSION_ROUTE_METRIC;
  data->Protocol = MIB_IPPROTO_NETMGMT;
  data->Loopback = false;
  data->AutoconfigureAddress = false;
  data->Publish = false;
  data->Immortal = false;
  data->Age = 0;
}

bool WindowsRouteMonitor::addExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Adding exclusion route for" << prefix.toString();
  return addExclusionRoutes({prefix}).size() == 1;
}

// Add a batch of exclusion routes. The routing table and interface metrics
// are fetched once per address family and every exclusion route is computed
// against that snapshot. This selects the same next hops as adding the routes
// one by one, because updateExclusionRoute() ignores routes of our own
// creation, but it avoids re-reading the (growing) routing table for every
// prefix, which made large split tunnelling lists take minutes to apply.
// Changes to the routing table made by others while the batch is running are
// handled by routeChanged(), like for any other exclusion route.
//
// Captured routes are updated once per family, after the new prefixes have
// been recorded, so that routes covered by the new exclusions are not cloned
// into the tunnel.
//
// Returns the list of prefixes that are now tracked as excluded, including
// non-routeable prefixes which are silently ignored.
QList<IPAddress> WindowsRouteMonitor::addExclusionRoutes(
    const QList<IPAddress>& prefixes) {
  QList<IPAddress> added;
  int created = 0;
  int failed = 0;
  QElapsedTimer timer;
  timer.start();

  for (int family : {AF_INET, AF_INET6}) {
    PMIB_IPFORWARD_TABLE2 table = nullptr;
    bool haveMetrics = false;

    for (const IPAddress& prefix : prefixes) {
      QHostAddress addr = prefix.address();
      int prefixFamily = AF_INET;
      if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        prefixFamily = AF_INET6;
      }
      if (prefixFamily != family) {
        continue;
      }

      // Silently ignore non-routeable addresses.
      if (addr.isLoopback() || addr.isBroadcast() || addr.isLinkLocal() ||
          addr.isMulticast()) {
        added.append(prefix);
        continue;
      }

      if (m_exclusionRoutes.contains(prefix)) {
        logger.warning() << "Exclusion route already exists";
        failed++;
        continue;
      }

      // Fetch the routing table and interface metrics once per family. If a
      // fetch fails, the next prefix tries again.
      if (table == nullptr) {
        DWORD result = GetIpForwardTable2(family, &table);
        if (result != NO_ERROR) {
          logger.error() << "Failed to fetch routing table:" << result;
          table = nullptr;
          failed++;
          continue;
        }
      }
      if (!haveMetrics) {
        haveMetrics = updateInterfaceMetrics(family);
      }

      MIB_IPFORWARD_ROW2* data = new MIB_IPFORWARD_ROW2;
      initExclusionRow(prefix, data);
      if (!updateExclusionRoute(data, table)) {
        failed++;
      } else if (data->InterfaceLuid.Value != 0) {
        created++;
      }

      m_exclusionRoutes[prefix] = data;
      added.append(prefix);
    }

    if (table != nullptr) {
      // Captured routes might have changed.
      updateCapturedRoutes(family, table);
      FreeMibTable(table);
    }
  }

  if (prefixes.size() > 1) {
    logger.debug() << "Added" << added.size() << "of" << prefixes.size()
                   << "exclusion routes (" << created << "installed,"
                   << failed << "failed) in" << timer.elapsed() << "ms";
  }
  return added;
}

bool WindowsRouteMonitor::deleteExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Deleting exclusion route for"
                 << prefix.address().toString();

  MIB_IPFORWARD_ROW2* data = m_exclusionRoutes.take(prefix);
  if (data == nullptr) {
    return true;
  }

  DWORD result = DeleteIpForwardEntry2(data);
  if ((result != ERROR_NOT_FOUND) && (result != NO_ERROR)) {
    logger.error() << "Failed to delete route to"
                   << prefix.toString()
                   << "result:" << result;
  }

  // Captured routes might have changed.
  updateCapturedRoutes(data->DestinationPrefix.Prefix.si_family);

  delete data;
  return true;
}

void WindowsRouteMonitor::flushRouteTable(
    QHash<IPAddress, MIB_IPFORWARD_ROW2*>& table) {
  for (auto i = table.begin(); i != table.end(); i++) {
    MIB_IPFORWARD_ROW2* data = i.value();
    DWORD result = DeleteIpForwardEntry2(data);
    if ((result != ERROR_NOT_FOUND) && (result != NO_ERROR)) {
      logger.error() << "Failed to delete route to"
                     << i.key().toString()
                     << "result:" << result;
    }
    delete data;
  }
  table.clear();
}

void WindowsRouteMonitor::setDetaultRouteCapture(bool enable) {
  m_defaultRouteCapture = enable;

  // Flush any captured routes when disabling the feature.
  if (!m_defaultRouteCapture) {
    flushRouteTable(m_clonedRoutes);
    return;
  }
}

void WindowsRouteMonitor::routeChanged() {
  logger.debug() << "Routes changed";

  PMIB_IPFORWARD_TABLE2 table;
  DWORD result = GetIpForwardTable2(AF_UNSPEC, &table);
  if (result != NO_ERROR) {
    logger.error() << "Failed to fetch routing table:" << result;
    return;
  }

  updateInterfaceMetrics(AF_UNSPEC);
  updateCapturedRoutes(AF_UNSPEC, table);
  for (MIB_IPFORWARD_ROW2* data : m_exclusionRoutes) {
    updateExclusionRoute(data, table);
  }

  FreeMibTable(table);
}
