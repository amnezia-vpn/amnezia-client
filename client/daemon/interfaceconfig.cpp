/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "interfaceconfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>

QJsonObject InterfaceConfig::toJson() const {
  QJsonObject json;
  QMetaEnum metaEnum = QMetaEnum::fromType<HopType>();

  json.insert("hopType", QJsonValue(metaEnum.valueToKey(m_hopType)));
  json.insert("privateKey", QJsonValue(m_privateKey));
  json.insert("deviceIpv4Address", QJsonValue(m_deviceIpv4Address));
  json.insert("deviceIpv6Address", QJsonValue(m_deviceIpv6Address));
  json.insert("serverPublicKey", QJsonValue(m_serverPublicKey));
  json.insert("serverPskKey", QJsonValue(m_serverPskKey));
  json.insert("serverIpv4AddrIn", QJsonValue(m_serverIpv4AddrIn));
  json.insert("serverIpv6AddrIn", QJsonValue(m_serverIpv6AddrIn));
  json.insert("serverPort", QJsonValue((double)m_serverPort));
  json.insert("deviceMTU", QJsonValue(m_deviceMTU));
  if ((m_hopType == InterfaceConfig::MultiHopExit) ||
      (m_hopType == InterfaceConfig::SingleHop)) {
    json.insert("serverIpv4Gateway", QJsonValue(m_serverIpv4Gateway));
    json.insert("serverIpv6Gateway", QJsonValue(m_serverIpv6Gateway));
    json.insert("dnsServer", QJsonValue(m_dnsServer));
  }

  QJsonArray allowedIPAddesses;
  for (const IPAddress& i : m_allowedIPAddressRanges) {
    QJsonObject range;
    range.insert("address", QJsonValue(i.address().toString()));
    range.insert("range", QJsonValue((double)i.prefixLength()));
    range.insert("isIpv6",
                 QJsonValue(i.type() == QAbstractSocket::IPv6Protocol));
    allowedIPAddesses.append(range);
  };
  json.insert("allowedIPAddressRanges", allowedIPAddesses);

  QJsonArray jsExcludedAddresses;
  for (const QString& i : m_excludedAddresses) {
    jsExcludedAddresses.append(QJsonValue(i));
  }
  json.insert("excludedAddresses", jsExcludedAddresses);

  QJsonArray disabledApps;
  for (const QString& i : m_vpnDisabledApps) {
    disabledApps.append(QJsonValue(i));
  }
  json.insert("vpnDisabledApps", disabledApps);

  return json;
}

QString InterfaceConfig::toWgConf(const QMap<QString, QString>& extra) const {
#define VALIDATE(x) \
  if (x.contains("\n")) return "";

  VALIDATE(m_privateKey);
  VALIDATE(m_deviceIpv4Address);
  VALIDATE(m_deviceIpv6Address);
  VALIDATE(m_serverIpv4Gateway);
  VALIDATE(m_serverIpv6Gateway);
  VALIDATE(m_serverPublicKey);
  VALIDATE(m_serverIpv4AddrIn);
  VALIDATE(m_serverIpv6AddrIn);
#undef VALIDATE

  QString content;
  QTextStream out(&content);
  out << "[Interface]\n";
  out << "PrivateKey = " << m_privateKey << "\n";

  QStringList addresses;
  if (!m_deviceIpv4Address.isNull()) {
    addresses.append(m_deviceIpv4Address);
  }
  if (!m_deviceIpv6Address.isNull()) {
    addresses.append(m_deviceIpv6Address);
  }
  if (addresses.isEmpty()) {
    return "";
  }

  out << "Address = " << addresses.join(", ") << "\n";

  if (m_deviceMTU) {
    out << "MTU = " << m_deviceMTU << "\n";
  }

  if (!m_dnsServer.isNull()) {
    QStringList dnsServers(m_dnsServer);
    // If the DNS is not the Gateway, it's a user defined DNS
    // thus, not add any other :)
    if (m_dnsServer == m_serverIpv4Gateway) {
      dnsServers.append(m_serverIpv6Gateway);
    }
    out << "DNS = " << dnsServers.join(", ") << "\n";
  }

  if (!m_junkPacketCount.isNull()) {
    out << "Jc = " << m_junkPacketCount << "\n";
  }
  if (!m_junkPacketMinSize.isNull()) {
    out << "JMin = " << m_junkPacketMinSize << "\n";
  }
  if (!m_junkPacketMaxSize.isNull()) {
    out << "JMax = " << m_junkPacketMaxSize << "\n";
  }
  if (!m_initPacketJunkSize.isNull()) {
    out << "S1 = " << m_initPacketJunkSize << "\n";
  }
  if (!m_responsePacketJunkSize.isNull()) {
    out << "S2 = " << m_responsePacketJunkSize << "\n";
  }
  if (!m_initPacketMagicHeader.isNull()) {
    out << "H1 = " << m_initPacketMagicHeader << "\n";
  }
  if (!m_responsePacketMagicHeader.isNull()) {
    out << "H2 = " << m_responsePacketMagicHeader << "\n";
  }
  if (!m_underloadPacketMagicHeader.isNull()) {
    out << "H3 = " << m_underloadPacketMagicHeader << "\n";
  }
  if (!m_transportPacketMagicHeader.isNull()) {
    out << "H4 = " << m_transportPacketMagicHeader << "\n";
  }

  // If any extra config was provided, append it now.
  for (const QString& key : extra.keys()) {
    out << key << " = " << extra[key] << "\n";
  }

  out << "\n[Peer]\n";
  out << "PublicKey = " << m_serverPublicKey << "\n";
  out << "Endpoint = " << m_serverIpv4AddrIn.toUtf8() << ":" << m_serverPort
      << "\n";

  /* In theory, we should use the ipv6 endpoint, but wireguard doesn't seem
   * to be happy if there are 2 endpoints.
  out << "Endpoint = [" << config.m_serverIpv6AddrIn << "]:"
      << config.m_serverPort << "\n";
  */
  QStringList ranges;
  for (const IPAddress& ip : m_allowedIPAddressRanges) {
    ranges.append(ip.toString());
  }
  out << "AllowedIPs = " << ranges.join(", ") << "\n";

  return content;
}
