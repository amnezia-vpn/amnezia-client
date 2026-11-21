/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsroutemonitor.h"

#include <QScopeGuard>

#include "leakdetector.h"
#include "logger.h"
#include "windowswcmsuspender.h"

namespace {
Logger logger("WindowsRouteMonitor");
};  // namespace

// Attempt to mark routing entries that we create with a relatively
// high metric. This ensures that we can skip over routes of our own
// creation when processing route changes, and ensures that we give
// way to other routing entries.
constexpr const ULONG EXCLUSION_ROUTE_METRIC = 0x5e72;

// Coalesce NotifyRouteChange2 bursts into a single rescan.
constexpr int ROUTE_CHANGE_DEBOUNCE_MSEC = 50;

// Called by the kernel on route changes - perform some basic filtering and
// schedule a coalesced rescan on the Qt thread.
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

static bool isNonRoutable(const QHostAddress& addr) {
  return addr.isLoopback() || addr.isBroadcast() || addr.isLinkLocal() ||
         addr.isMulticast();
}

static bool isOwnExclusionRoute(const MIB_IPFORWARD_ROW2* row) {
  return (row->Protocol == MIB_IPPROTO_NETMGMT) &&
         (row->Metric == EXCLUSION_ROUTE_METRIC);
}

static bool isUnspecifiedNextHop(const SOCKADDR_INET& hop) {
  if (hop.si_family == AF_INET) {
    return hop.Ipv4.sin_addr.s_addr == 0;
  }
  if (hop.si_family == AF_INET6) {
    static const quint8 zero[16] = {};
    return memcmp(&hop.Ipv6.sin6_addr, zero, sizeof(zero)) == 0;
  }
  return true;
}

WindowsRouteMonitor::WindowsRouteMonitor(quint64 luid, QObject* parent)
    : QObject(parent), m_luid(luid) {
  MZ_COUNT_CTOR(WindowsRouteMonitor);
  logger.debug() << "WindowsRouteMonitor created.";

  m_routeChangedDebounce.setSingleShot(true);
  m_routeChangedDebounce.setInterval(ROUTE_CHANGE_DEBOUNCE_MSEC);
  connect(&m_routeChangedDebounce, &QTimer::timeout, this,
          &WindowsRouteMonitor::routeChanged);

  NotifyRouteChange2(AF_INET, routeChangeCallback, this, FALSE, &m_routeHandle);
}

WindowsRouteMonitor::~WindowsRouteMonitor() {
  MZ_COUNT_DTOR(WindowsRouteMonitor);
  CancelMibChangeNotify2(m_routeHandle);
  m_routeChangedDebounce.stop();

  flushRouteTable(m_exclusionRoutes);
  flushRouteTable(m_clonedRoutes);
  logger.debug() << "WindowsRouteMonitor destroyed.";
}

void WindowsRouteMonitor::updateInterfaceMetrics(int family) {
  PMIB_IPINTERFACE_TABLE table;
  DWORD result = GetIpInterfaceTable(family, &table);
  if (result != NO_ERROR) {
    logger.warning() << "Failed to retrive interface table." << result;
    return;
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
}

// Possible next hops for exclusions: routes outside the tunnel, not ours.
WindowsRouteMonitor::RowList WindowsRouteMonitor::exclusionCandidates(
    void* ptable, int family) const {
  PMIB_IPFORWARD_TABLE2 table = reinterpret_cast<PMIB_IPFORWARD_TABLE2>(ptable);
  RowList rows;
  for (ULONG i = 0; i < table->NumEntries; i++) {
    const MIB_IPFORWARD_ROW2* row = &table->Table[i];
    if (row->InterfaceLuid.Value == m_luid) {
      continue;
    }
    if (isOwnExclusionRoute(row)) {
      continue;
    }
    if (row->DestinationPrefix.Prefix.si_family != family) {
      continue;
    }
    rows.append(row);
  }
  return rows;
}

WindowsRouteMonitor::NextHop WindowsRouteMonitor::resolveExclusionHop(
    const MIB_IPFORWARD_ROW2* data, const RowList& candidates) const {
  NextHop best;
  int bestMatch = -1;
  ULONG bestMetric = ULONG_MAX;
  best.address.si_family = data->DestinationPrefix.Prefix.si_family;

  const QMap<quint64, ULONG>& interfaceMetrics =
      (data->DestinationPrefix.Prefix.si_family == AF_INET6)
          ? m_interfaceMetricsIpv6
          : m_interfaceMetricsIpv4;

  for (const MIB_IPFORWARD_ROW2* row : candidates) {
    if (row->DestinationPrefix.PrefixLength < bestMatch) {
      continue;
    }

    // Check if the routing table entry matches the destination.
    if (!routeContainsDest(&row->DestinationPrefix, &data->DestinationPrefix)) {
      continue;
    }

    // Compute the combined interface and routing metric.
    auto metric = interfaceMetrics.constFind(row->InterfaceLuid.Value);
    if (metric == interfaceMetrics.constEnd()) {
      continue;
    }
    ULONG routeMetric = row->Metric + metric.value();
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
    best.address = row->NextHop;
    bestMatch = row->DestinationPrefix.PrefixLength;
    bestMetric = routeMetric;
    if (bestMatch == data->DestinationPrefix.PrefixLength) {
      best.luid = 0;  // Don't write to the table if we find an exact match.
    } else {
      best.luid = row->InterfaceLuid.Value;
    }
  }
  return best;
}

// static
bool WindowsRouteMonitor::hopChanged(const MIB_IPFORWARD_ROW2* data,
                                     const NextHop& hop) {
  return data->InterfaceLuid.Value != hop.luid ||
         memcmp(&hop.address, &data->NextHop, sizeof(SOCKADDR_INET)) != 0;
}

void WindowsRouteMonitor::applyExclusionHop(MIB_IPFORWARD_ROW2* data,
                                            const NextHop& hop) {
  // Delete the previous routing table entry, if any.
  if (data->InterfaceLuid.Value != 0) {
    DWORD result = DeleteIpForwardEntry2(data);
    if ((result != NO_ERROR) && (result != ERROR_NOT_FOUND)) {
      logger.error() << "Failed to delete route:" << result;
    }
  }

  // Update the routing table entry.
  data->InterfaceLuid.Value = hop.luid;
  data->NextHop = hop.address;
  if (data->InterfaceLuid.Value != 0) {
    DWORD result = CreateIpForwardEntry2(data);
    if (result != NO_ERROR) {
      logger.error() << "Failed to update route:" << result;
    }
  }
}

void WindowsRouteMonitor::updateExclusionRoute(MIB_IPFORWARD_ROW2* data,
                                               void* table) {
  RowList candidates =
      exclusionCandidates(table, data->DestinationPrefix.Prefix.si_family);
  NextHop hop = resolveExclusionHop(data, candidates);
  if (hopChanged(data, hop)) {
    applyExclusionHop(data, hop);
  }
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

  // Cloning these would fail with ERROR_OBJECT_ALREADY_EXISTS.
  QSet<IPAddress> tunnelPrefixes;
  for (ULONG i = 0; i < table->NumEntries; i++) {
    const MIB_IPFORWARD_ROW2* row = &table->Table[i];
    if (row->InterfaceLuid.Value == m_luid &&
        isUnspecifiedNextHop(row->NextHop)) {
      tunnelPrefixes.insert(IPAddress(prefixToAddress(&row->DestinationPrefix),
                                      row->DestinationPrefix.PrefixLength));
    }
  }

  QSet<IPAddress> capturable;
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
    if (isOwnExclusionRoute(row)) {
      continue;
    }
    // Ignore routes which should be excluded.
    if (isRouteExcluded(&row->DestinationPrefix)) {
      continue;
    }
    QHostAddress destination = prefixToAddress(&row->DestinationPrefix);
    if (isNonRoutable(destination)) {
      continue;
    }

    // If we get here, this route should be cloned.
    IPAddress prefix(destination, row->DestinationPrefix.PrefixLength);
    capturable.insert(prefix);
    MIB_IPFORWARD_ROW2* data = m_clonedRoutes.value(prefix, nullptr);
    if (data != nullptr) {
      // Count the number of matching entries in the main table.
      data->Age++;
      continue;
    }
    if (tunnelPrefixes.contains(prefix) || m_captureRefused.contains(prefix)) {
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
    if (result == NO_ERROR) {
      m_clonedRoutes.insert(prefix, data);
      data->Age++;
      continue;
    }
    delete data;
    if (result == ERROR_OBJECT_ALREADY_EXISTS) {
      logger.debug() << "Route to" << prefix.toString()
                     << "already exists, not capturing";
      m_captureRefused.insert(prefix);
    } else {
      logger.error() << "Failed to update route:" << result;
    }
  }

  // Retry refused prefixes once their source route is gone.
  for (auto it = m_captureRefused.begin(); it != m_captureRefused.end();) {
    const bool sameFamily =
        (family == AF_UNSPEC) ||
        (family == AF_INET6) == (it->type() == QAbstractSocket::IPv6Protocol);
    if (sameFamily && !capturable.contains(*it)) {
      it = m_captureRefused.erase(it);
    } else {
      ++it;
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
MIB_IPFORWARD_ROW2* WindowsRouteMonitor::buildExclusionRow(
    const IPAddress& prefix) {
  // Allocate and initialize the MIB routing table row.
  MIB_IPFORWARD_ROW2* data = new MIB_IPFORWARD_ROW2;
  InitializeIpForwardEntry(data);
  if (prefix.address().protocol() == QAbstractSocket::IPv6Protocol) {
    Q_IPV6ADDR buf = prefix.address().toIPv6Address();

    memcpy(&data->DestinationPrefix.Prefix.Ipv6.sin6_addr, &buf, sizeof(buf));
    data->DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
  } else {
    quint32 buf = prefix.address().toIPv4Address();

    data->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = htonl(buf);
    data->DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
  }
  data->DestinationPrefix.PrefixLength = prefix.prefixLength();
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
  return data;
}

bool WindowsRouteMonitor::addExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Adding exclusion route for" << prefix.toString();

  // Silently ignore non-routeable addresses.
  if (isNonRoutable(prefix.address())) {
    return true;
  }

  if (m_exclusionRoutes.contains(prefix)) {
    logger.warning() << "Exclusion route already exists";
    return false;
  }

  MIB_IPFORWARD_ROW2* data = buildExclusionRow(prefix);
  int family = data->DestinationPrefix.Prefix.si_family;

  PMIB_IPFORWARD_TABLE2 table;
  DWORD result = GetIpForwardTable2(family, &table);
  if (result != NO_ERROR) {
    logger.error() << "Failed to fetch routing table:" << result;
    delete data;
    return false;
  }
  updateInterfaceMetrics(family);
  updateCapturedRoutes(family, table);
  updateExclusionRoute(data, table);
  FreeMibTable(table);

  m_exclusionRoutes[prefix] = data;
  return true;
}

int WindowsRouteMonitor::addExclusionRoutes(const QList<IPAddress>& prefixes) {
  if (prefixes.isEmpty()) {
    return 0;
  }

  QList<QPair<IPAddress, MIB_IPFORWARD_ROW2*>> pendingV4;
  QList<QPair<IPAddress, MIB_IPFORWARD_ROW2*>> pendingV6;

  for (const IPAddress& prefix : prefixes) {
    if (isNonRoutable(prefix.address())) {
      continue;
    }
    if (m_exclusionRoutes.contains(prefix)) {
      continue;
    }

    MIB_IPFORWARD_ROW2* data = buildExclusionRow(prefix);
    if (data->DestinationPrefix.Prefix.si_family == AF_INET6) {
      pendingV6.append(qMakePair(prefix, data));
    } else {
      pendingV4.append(qMakePair(prefix, data));
    }
    // Register first so updateCapturedRoutes() below sees all exclusions.
    m_exclusionRoutes[prefix] = data;
  }

  if (pendingV4.isEmpty() && pendingV6.isEmpty()) {
    return 0;
  }

  logger.debug() << "Batch exclusion install:" << pendingV4.size() << "IPv4,"
                 << pendingV6.size() << "IPv6";

  WcmSuspender suspender(pendingV4.size() + pendingV6.size());
  int added = 0;

  auto processFamily =
      [&](int family,
          const QList<QPair<IPAddress, MIB_IPFORWARD_ROW2*>>& pending) {
        if (pending.isEmpty()) {
          return;
        }

        PMIB_IPFORWARD_TABLE2 table = nullptr;
        DWORD result = GetIpForwardTable2(family, &table);
        if (result != NO_ERROR) {
          logger.error() << "Failed to fetch routing table:" << result;
          for (const auto& item : pending) {
            delete m_exclusionRoutes.take(item.first);
          }
          return;
        }

        updateInterfaceMetrics(family);
        RowList candidates = exclusionCandidates(table, family);
        for (const auto& item : pending) {
          NextHop hop = resolveExclusionHop(item.second, candidates);
          if (hopChanged(item.second, hop)) {
            applyExclusionHop(item.second, hop);
          }
          ++added;
        }
        updateCapturedRoutes(family, table);

        FreeMibTable(table);
      };

  processFamily(AF_INET, pendingV4);
  processFamily(AF_INET6, pendingV6);

  return added;
}

void WindowsRouteMonitor::removeRoute(MIB_IPFORWARD_ROW2* data,
                                      const IPAddress& prefix) {
  // Never installed: resolved to an exact system route.
  if (data->InterfaceLuid.Value == 0) {
    return;
  }
  DWORD result = DeleteIpForwardEntry2(data);
  if ((result != ERROR_NOT_FOUND) && (result != NO_ERROR)) {
    logger.error() << "Failed to delete route to" << prefix.toString()
                   << "result:" << result;
  }
}

bool WindowsRouteMonitor::deleteExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Deleting exclusion route for"
                 << prefix.address().toString();

  MIB_IPFORWARD_ROW2* data = m_exclusionRoutes.take(prefix);
  if (data == nullptr) {
    return true;
  }

  removeRoute(data, prefix);

  // Captured routes might have changed.
  updateCapturedRoutes(data->DestinationPrefix.Prefix.si_family);

  delete data;
  return true;
}

int WindowsRouteMonitor::deleteExclusionRoutes(
    const QList<IPAddress>& prefixes) {
  QList<QPair<IPAddress, MIB_IPFORWARD_ROW2*>> removed;
  for (const IPAddress& prefix : prefixes) {
    MIB_IPFORWARD_ROW2* data = m_exclusionRoutes.take(prefix);
    if (data != nullptr) {
      removed.append(qMakePair(prefix, data));
    }
  }
  if (removed.isEmpty()) {
    return 0;
  }

  logger.debug() << "Batch exclusion removal:" << removed.size() << "routes";

  bool touchedV4 = false;
  bool touchedV6 = false;
  {
    WcmSuspender suspender(removed.size());
    for (const auto& item : removed) {
      removeRoute(item.second, item.first);
      if (item.second->DestinationPrefix.Prefix.si_family == AF_INET6) {
        touchedV6 = true;
      } else {
        touchedV4 = true;
      }
      delete item.second;
    }
  }

  // Captured routes might have changed.
  if (touchedV4) {
    updateCapturedRoutes(AF_INET);
  }
  if (touchedV6) {
    updateCapturedRoutes(AF_INET6);
  }
  return removed.size();
}

void WindowsRouteMonitor::flushRouteTable(
    QHash<IPAddress, MIB_IPFORWARD_ROW2*>& table) {
  WcmSuspender suspender(table.size());
  for (auto i = table.begin(); i != table.end(); i++) {
    MIB_IPFORWARD_ROW2* data = i.value();
    removeRoute(data, i.key());
    delete data;
  }
  table.clear();
}

void WindowsRouteMonitor::setDetaultRouteCapture(bool enable) {
  m_defaultRouteCapture = enable;

  // Flush any captured routes when disabling the feature.
  if (!m_defaultRouteCapture) {
    flushRouteTable(m_clonedRoutes);
    m_captureRefused.clear();
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

  PMIB_IPFORWARD_TABLE2 table;
  DWORD result = GetIpForwardTable2(AF_UNSPEC, &table);
  if (result != NO_ERROR) {
    logger.error() << "Failed to fetch routing table:" << result;
    return;
  }
  auto guard = qScopeGuard([&] { FreeMibTable(table); });

  updateInterfaceMetrics(AF_UNSPEC);

  // Resolve first, so a mass move can run with wcmSvc suspended.
  const RowList candidatesV4 = exclusionCandidates(table, AF_INET);
  const RowList candidatesV6 = exclusionCandidates(table, AF_INET6);
  QList<QPair<MIB_IPFORWARD_ROW2*, NextHop>> moved;
  for (MIB_IPFORWARD_ROW2* data : std::as_const(m_exclusionRoutes)) {
    const RowList& candidates =
        (data->DestinationPrefix.Prefix.si_family == AF_INET6) ? candidatesV6
                                                               : candidatesV4;
    NextHop hop = resolveExclusionHop(data, candidates);
    if (hopChanged(data, hop)) {
      moved.append(qMakePair(data, hop));
    }
  }

  WcmSuspender suspender(moved.size() * 2);  // delete + create each
  updateCapturedRoutes(AF_UNSPEC, table);
  for (const auto& item : moved) {
    applyExclusionHop(item.first, item.second);
  }
}
