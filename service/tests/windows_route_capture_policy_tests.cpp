/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsroutecapturepolicy.h"

#include <algorithm>
#include <initializer_list>

#include <QtTest/QtTest>

namespace {

constexpr quint64 VPN_LUID = 42;
constexpr quint64 LAN_LUID = 7;
constexpr ULONG MONITOR_METRIC = 0x5e72;

MIB_IPFORWARD_ROW2 makeRoute(ADDRESS_FAMILY family = AF_INET) {
  MIB_IPFORWARD_ROW2 row;
  InitializeIpForwardEntry(&row);
  row.InterfaceLuid.Value = LAN_LUID;
  row.DestinationPrefix.PrefixLength = 24;
  row.DestinationPrefix.Prefix.si_family = family;
  row.NextHop.si_family = family;
  row.Protocol = MIB_IPPROTO_NETMGMT;
  row.Metric = 10;
  return row;
}

void setIpv4NextHop(MIB_IPFORWARD_ROW2& row, ULONG address) {
  row.NextHop.si_family = AF_INET;
  row.NextHop.Ipv4.sin_addr.s_addr = htonl(address);
}

void setIpv6NextHop(MIB_IPFORWARD_ROW2& row, const IN6_ADDR& address) {
  row.NextHop.si_family = AF_INET6;
  row.NextHop.Ipv6.sin6_addr = address;
}

IN6_ADDR ipv6Address(std::initializer_list<unsigned char> bytes) {
  IN6_ADDR address = {};
  std::copy(bytes.begin(), bytes.end(), address.u.Byte);
  return address;
}

}  // namespace

class WindowsRouteCapturePolicyTests final : public QObject {
  Q_OBJECT

 private slots:
  void capturesRoutedIpv4Prefix();
  void capturesRoutedIpv6Prefix();
  void skipsVpnInterfaceRoutes();
  void skipsDefaultRoutes();
  void skipsRoutesCreatedByMonitor();
  void capturesSameMetricRoutesNotCreatedByMonitor();
  void skipsExcludedRoutes();
  void skipsOnLinkIpv4Routes();
  void skipsOnLinkIpv6Routes();
  void skipsUnspecifiedNextHopRoutes();
};

void WindowsRouteCapturePolicyTests::capturesRoutedIpv4Prefix() {
  auto row = makeRoute();
  setIpv4NextHop(row, 0xc0a80101);

  QVERIFY(WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::capturesRoutedIpv6Prefix() {
  auto row = makeRoute(AF_INET6);
  row.DestinationPrefix.PrefixLength = 64;
  setIpv6NextHop(row, ipv6Address({0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 1}));

  QVERIFY(!WindowsRouteCapturePolicy::isOnLinkRoute(&row));
  QVERIFY(WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsVpnInterfaceRoutes() {
  auto row = makeRoute();
  row.InterfaceLuid.Value = VPN_LUID;
  setIpv4NextHop(row, 0xc0a80101);

  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsDefaultRoutes() {
  auto row = makeRoute();
  row.DestinationPrefix.PrefixLength = 0;
  setIpv4NextHop(row, 0xc0a80101);

  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsRoutesCreatedByMonitor() {
  auto row = makeRoute();
  row.Metric = MONITOR_METRIC;
  setIpv4NextHop(row, 0xc0a80101);

  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::capturesSameMetricRoutesNotCreatedByMonitor() {
  auto row = makeRoute();
  row.Protocol = static_cast<NL_ROUTE_PROTOCOL>(0);
  row.Metric = MONITOR_METRIC;
  setIpv4NextHop(row, 0xc0a80101);

  QVERIFY(!WindowsRouteCapturePolicy::isRouteCreatedByMonitor(
      &row, MONITOR_METRIC));
  QVERIFY(WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsExcludedRoutes() {
  auto row = makeRoute();
  setIpv4NextHop(row, 0xc0a80101);

  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, true, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsOnLinkIpv4Routes() {
  auto row = makeRoute();
  setIpv4NextHop(row, 0);

  QVERIFY(WindowsRouteCapturePolicy::isOnLinkRoute(&row));
  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsOnLinkIpv6Routes() {
  auto row = makeRoute(AF_INET6);
  IN6_ADDR unspecified = {};
  setIpv6NextHop(row, unspecified);

  QVERIFY(WindowsRouteCapturePolicy::isOnLinkRoute(&row));
  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

void WindowsRouteCapturePolicyTests::skipsUnspecifiedNextHopRoutes() {
  auto row = makeRoute();
  row.NextHop.si_family = AF_UNSPEC;

  QVERIFY(WindowsRouteCapturePolicy::isOnLinkRoute(&row));
  QVERIFY(!WindowsRouteCapturePolicy::shouldCaptureRoute(
      &row, VPN_LUID, false, MONITOR_METRIC));
}

QTEST_APPLESS_MAIN(WindowsRouteCapturePolicyTests)

#include "windows_route_capture_policy_tests.moc"
