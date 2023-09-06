/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef INTERFACECONFIG_H
#define INTERFACECONFIG_H

#include <QList>
#include <QString>

#include "ipaddress.h"

class QJsonObject;

class InterfaceConfig {
  Q_GADGET

 public:
  InterfaceConfig() {}

  enum HopType { SingleHop, MultiHopEntry, MultiHopExit };
  Q_ENUM(HopType)

  HopType m_hopType;
  QString m_privateKey;
  QString m_deviceIpv4Address;
  QString m_deviceIpv6Address;
  QString m_serverIpv4Gateway;
  QString m_serverIpv6Gateway;
  QString m_serverPublicKey;
  QString m_serverIpv4AddrIn;
  QString m_serverIpv6AddrIn;
  QString m_dnsServer;
  int m_serverPort = 0;
  QList<IPAddress> m_allowedIPAddressRanges;
  QStringList m_excludedAddresses;
  QStringList m_vpnDisabledApps;
#if defined(MZ_ANDROID) || defined(MZ_IOS)
  QString m_installationId;
#endif

  QJsonObject toJson() const;
  QString toWgConf(
      const QMap<QString, QString>& extra = QMap<QString, QString>()) const;
};

#endif  // INTERFACECONFIG_H
