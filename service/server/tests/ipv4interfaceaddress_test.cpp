/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QtTest>

#include "ipv4interfaceaddress.h"

class IPv4InterfaceAddressTest : public QObject {
  Q_OBJECT

 private slots:
  void parsesIPv4FromCombinedWireGuardAddress();
};

void IPv4InterfaceAddressTest::parsesIPv4FromCombinedWireGuardAddress() {
  // WireGuard allows IPv4 and IPv6 interface addresses in the same
  // comma-separated Address value.
  const QString configuredAddresses =
      "10.8.0.4/24, 2001:db8::9e/128";

  const auto parsed = parseIPv4InterfaceAddress(configuredAddresses);

  // Keep the configured host (10.8.0.4), rather than normalizing it to the
  // subnet address (10.8.0.0), and preserve its prefix length.
  QVERIFY(parsed.has_value());
  QCOMPARE(parsed->address, QHostAddress("10.8.0.4"));
  QCOMPARE(parsed->prefixLength, 24);
}

QTEST_APPLESS_MAIN(IPv4InterfaceAddressTest)

#include "ipv4interfaceaddress_test.moc"
