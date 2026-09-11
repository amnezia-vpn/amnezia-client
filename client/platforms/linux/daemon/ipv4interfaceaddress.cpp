/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ipv4interfaceaddress.h"

#include <QStringList>

std::optional<ParsedIPv4InterfaceAddress> parseIPv4InterfaceAddress(
    const QString& configuredAddresses) {
  for (const QString& entry : configuredAddresses.split(',')) {
    const QStringList addressAndPrefix = entry.trimmed().split('/');
    if (addressAndPrefix.size() != 2) continue;

    QHostAddress candidateAddress(addressAndPrefix.at(0));
    bool prefixIsValid = false;
    const int prefixLength = addressAndPrefix.at(1).toInt(&prefixIsValid);
    if (candidateAddress.protocol() != QAbstractSocket::IPv4Protocol ||
        !prefixIsValid || prefixLength < 0 || prefixLength > 32) {
      continue;
    }

    return ParsedIPv4InterfaceAddress{candidateAddress, prefixLength};
  }

  return std::nullopt;
}
