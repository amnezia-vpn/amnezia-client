/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsroutemonitor.h"

#include <QScopeGuard>
#include <QVector>

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

constexpr int ROUTE_CHANGE_DEBOUNCE_MSEC = 50;

static void routeChangeCallback(PVOID context, PMIB_IPFORWARD_ROW2 row,
                                MIB_NOTIFICATION_TYPE type) {
  WindowsRouteMonitor* monitor = (WindowsRouteMonitor*)context;
  Q_UNUSED(type);

  // Ignore route changes that we created.
  if (row != nullptr) {
    if ((row->Protocol == MIB_IPPROTO_NETMGMT) &&
        (row->Metric == EXCLUSION_ROUTE_METRIC)) {
      return;
    }
    if (monitor->getLuid() == row->InterfaceLuid.Value) {
      return;
    }
  }

  QMetaObject::invokeMethod(monitor, "scheduleRouteChanged",
                            Qt::QueuedConnection);
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

  m_routeChangedDebounce.setSingleShot(true);
  m_routeChangedDebounce.setInterval(ROUTE_CHANGE_DEBOUNCE_MSEC);
  connect(&m_routeChangedDebounce, &QTimer::timeout, this,
          &WindowsRouteMonitor::routeChanged);

  DWORD result =
      NotifyRouteChange2(AF_INET, routeChangeCallback, this, FALSE,
                         &m_routeHandle);
  if (result != NO_ERROR) {
    logger.error() << "Failed to subscribe to route changes:" << result;
    m_routeHandle = INVALID_HANDLE_VALUE;
  }
}

WindowsRouteMonitor::~WindowsRouteMonitor() {
  MZ_COUNT_DTOR(WindowsRouteMonitor);
  if (m_routeHandle != INVALID_HANDLE_VALUE) {
    CancelMibChangeNotify2(m_routeHandle);
  }

  flushRouteTable(m_exclusionRoutes);
  flushRouteTable(m_clonedRoutes);
  logger.debug() << "WindowsRouteMonitor destroyed.";
}

bool WindowsRouteMonitor::updateInterfaceMetrics(int family) {
  PMIB_IPINTERFACE_TABLE table = nullptr;
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

  if (bestMatch < 0) {
    logger.error() << "No physical route found for exclusion"
                   << prefixToAddress(&data->DestinationPrefix).toString()
                   << "/" << data->DestinationPrefix.PrefixLength;
    return false;
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
      return false;
    }
  }

  // Update the routing table entry.
  data->InterfaceLuid.Value = bestLuid;
  data->NextHop = nexthop;
  if (data->InterfaceLuid.Value != 0) {
    DWORD result = CreateIpForwardEntry2(data);
    if (result == ERROR_OBJECT_ALREADY_EXISTS) {
      logger.warning() << "Exclusion route is already owned by another entry";
      data->InterfaceLuid.Value = 0;
      return true;
    }
    if (result != NO_ERROR) {
      logger.error() << "Failed to update route:" << result;
      data->InterfaceLuid.Value = 0;
      data->NextHop = {};
      data->NextHop.si_family = data->DestinationPrefix.Prefix.si_family;
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

  PMIB_IPFORWARD_TABLE2 table = nullptr;
  DWORD error = GetIpForwardTable2(family, &table);
  if (error != NO_ERROR) {
    logger.error() << "Failed to fetch routing table:" << error;
    return;
  }

  updateCapturedRoutes(family, table);
  FreeMibTable(table);
}

void WindowsRouteMonitor::updateCapturedRoutes(int family, void* ptable) {
  PMIB_IPFORWARD_TABLE2 table = reinterpret_cast<PMIB_IPFORWARD_TABLE2>(ptable);
  if (!m_defaultRouteCapture) {
    return;
  }

  QSet<IPAddress> captureFailures;
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
    if (captureFailures.contains(prefix)) {
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
      if (result == ERROR_OBJECT_ALREADY_EXISTS) {
        captureFailures.insert(prefix);
      }
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

bool WindowsRouteMonitor::addExclusionRoute(const IPAddress& prefix) {
  return addExclusionRoutes(QList<IPAddress>{prefix});
}

bool WindowsRouteMonitor::addExclusionRoutes(
    const QList<IPAddress>& prefixes) {
  using PendingRoute = QPair<IPAddress, MIB_IPFORWARD_ROW2*>;

  QVector<PendingRoute> pendingIpv4;
  QVector<PendingRoute> pendingIpv6;
  pendingIpv4.reserve(prefixes.size());
  pendingIpv6.reserve(prefixes.size());
  QSet<IPAddress> pendingPrefixes;

  auto releasePending = [](QVector<PendingRoute>& pending) {
    for (const PendingRoute& route : pending) {
      delete route.second;
    }
    pending.clear();
  };

  for (const IPAddress& prefix : prefixes) {
    const QHostAddress address = prefix.address();
    const auto protocol = address.protocol();
    if (protocol != QAbstractSocket::IPv4Protocol &&
        protocol != QAbstractSocket::IPv6Protocol) {
      logger.error() << "Invalid exclusion prefix:" << prefix.toString();
      releasePending(pendingIpv4);
      releasePending(pendingIpv6);
      return false;
    }

    // Silently ignore non-routeable addresses.
    if (address.isLoopback() || address.isBroadcast() ||
        address.isLinkLocal() || address.isMulticast()) {
      continue;
    }
    if (m_exclusionRoutes.contains(prefix) ||
        pendingPrefixes.contains(prefix)) {
      continue;
    }

    MIB_IPFORWARD_ROW2* data = new MIB_IPFORWARD_ROW2;
    InitializeIpForwardEntry(data);
    if (protocol == QAbstractSocket::IPv6Protocol) {
      const Q_IPV6ADDR buffer = address.toIPv6Address();
      static_assert(
          sizeof(buffer.c) ==
          sizeof(data->DestinationPrefix.Prefix.Ipv6.sin6_addr.s6_addr));
      for (int index = 0; index < static_cast<int>(sizeof(buffer.c)); ++index) {
        data->DestinationPrefix.Prefix.Ipv6.sin6_addr.s6_addr[index] =
            buffer[index];
      }
      data->DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
    } else {
      quint32 buffer = address.toIPv4Address();
      data->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = htonl(buffer);
      data->DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
    }
    data->DestinationPrefix.PrefixLength = prefix.prefixLength();
    data->NextHop.si_family = data->DestinationPrefix.Prefix.si_family;
    data->ValidLifetime = 0xffffffff;
    data->PreferredLifetime = 0xffffffff;
    data->Metric = EXCLUSION_ROUTE_METRIC;
    data->Protocol = MIB_IPPROTO_NETMGMT;
    data->Loopback = false;
    data->AutoconfigureAddress = false;
    data->Publish = false;
    data->Immortal = false;
    data->Age = 0;

    PendingRoute route(prefix, data);
    if (protocol == QAbstractSocket::IPv6Protocol) {
      pendingIpv6.append(route);
    } else {
      pendingIpv4.append(route);
    }
    pendingPrefixes.insert(prefix);
  }

  if (pendingIpv4.isEmpty() && pendingIpv6.isEmpty()) {
    return true;
  }

  for (const PendingRoute& route : pendingIpv4) {
    m_exclusionRoutes.insert(route.first, route.second);
  }
  for (const PendingRoute& route : pendingIpv6) {
    m_exclusionRoutes.insert(route.first, route.second);
  }

  auto processFamily = [this](int family,
                              const QVector<PendingRoute>& pending) {
    if (pending.isEmpty()) {
      return true;
    }

    logger.debug() << "Batch exclusion install:" << pending.size()
                   << (family == AF_INET6 ? "IPv6" : "IPv4") << "routes";

    PMIB_IPFORWARD_TABLE2 table = nullptr;
    DWORD result = GetIpForwardTable2(family, &table);
    if (result != NO_ERROR) {
      logger.error() << "Failed to fetch routing table:" << result;
      return false;
    }
    auto tableGuard = qScopeGuard([table] { FreeMibTable(table); });

    if (!updateInterfaceMetrics(family)) {
      return false;
    }
    for (const PendingRoute& route : pending) {
      if (!updateExclusionRoute(route.second, table)) {
        logger.error() << "Failed to install exclusion route"
                       << route.first.toString();
        return false;
      }
    }

    updateCapturedRoutes(family, table);
    return true;
  };

  const bool installed = processFamily(AF_INET, pendingIpv4) &&
                         processFamily(AF_INET6, pendingIpv6);
  if (installed) {
    return true;
  }

  auto rollback = [this](const QVector<PendingRoute>& pending) {
    for (const PendingRoute& route : pending) {
      MIB_IPFORWARD_ROW2* data = m_exclusionRoutes.take(route.first);
      if (data == nullptr) {
        continue;
      }
      if (data->InterfaceLuid.Value != 0) {
        DWORD result = DeleteIpForwardEntry2(data);
        if (result != NO_ERROR && result != ERROR_NOT_FOUND) {
          logger.error() << "Failed to roll back exclusion route"
                         << route.first.toString() << "result:" << result;
          m_exclusionRoutes.insert(route.first, data);
          continue;
        }
      }
      delete data;
    }
  };
  rollback(pendingIpv4);
  rollback(pendingIpv6);

  if (!pendingIpv4.isEmpty()) {
    updateCapturedRoutes(AF_INET);
  }
  if (!pendingIpv6.isEmpty()) {
    updateCapturedRoutes(AF_INET6);
  }

  return false;
}

bool WindowsRouteMonitor::deleteExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Deleting exclusion route for"
                 << prefix.address().toString();

  MIB_IPFORWARD_ROW2* data = m_exclusionRoutes.take(prefix);
  if (data == nullptr) {
    return true;
  }

  if (data->InterfaceLuid.Value != 0) {
    DWORD result = DeleteIpForwardEntry2(data);
    if ((result != ERROR_NOT_FOUND) && (result != NO_ERROR)) {
      logger.error() << "Failed to delete route to" << prefix.toString()
                     << "result:" << result;
      m_exclusionRoutes.insert(prefix, data);
      return false;
    }
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
    if (data->InterfaceLuid.Value != 0) {
      DWORD result = DeleteIpForwardEntry2(data);
      if ((result != ERROR_NOT_FOUND) && (result != NO_ERROR)) {
        logger.error() << "Failed to delete route to" << i.key().toString()
                       << "result:" << result;
      }
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

void WindowsRouteMonitor::scheduleRouteChanged() {
  if (!m_routeChangedDebounce.isActive()) {
    m_routeChangedDebounce.start();
  }
}

void WindowsRouteMonitor::routeChanged() {
  logger.debug() << "Routes changed";

  PMIB_IPFORWARD_TABLE2 table = nullptr;
  DWORD result = GetIpForwardTable2(AF_UNSPEC, &table);
  if (result != NO_ERROR) {
    logger.error() << "Failed to fetch routing table:" << result;
    return;
  }
  auto tableGuard = qScopeGuard([table] { FreeMibTable(table); });

  if (!updateInterfaceMetrics(AF_UNSPEC)) {
    return;
  }
  updateCapturedRoutes(AF_UNSPEC, table);
  for (MIB_IPFORWARD_ROW2* data : m_exclusionRoutes) {
    if (!updateExclusionRoute(data, table)) {
      logger.error() << "Failed to refresh an exclusion route";
    }
  }
}
