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
  if (!m_persistentKeepalive.isEmpty()) {
    json.insert("persistentKeepalive", QJsonValue(m_persistentKeepalive));
  }
  if ((m_hopType == InterfaceConfig::MultiHopExit) ||
      (m_hopType == InterfaceConfig::SingleHop)) {
    json.insert("serverIpv4Gateway", QJsonValue(m_serverIpv4Gateway));
    json.insert("serverIpv6Gateway", QJsonValue(m_serverIpv6Gateway));
    json.insert("primaryDnsServer", QJsonValue(m_primaryDnsServer));
    json.insert("secondaryDnsServer", QJsonValue(m_secondaryDnsServer));
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


  QJsonArray jsAllowedDnsServers;
  for (const QString& i : m_allowedDnsServers) {
    jsAllowedDnsServers.append(QJsonValue(i));
  }
  json.insert("allowedDnsServers", jsAllowedDnsServers);

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

  if (!m_primaryDnsServer.isEmpty()) {
    QStringList dnsServers;
    dnsServers.append(m_primaryDnsServer);
    if (!m_secondaryDnsServer.isEmpty()) {
        dnsServers.append(m_secondaryDnsServer);
    }
    // If the DNS is not the Gateway, it's a user defined DNS
    // thus, not add any other :)
    if (m_primaryDnsServer == m_serverIpv4Gateway) {
      dnsServers.append(m_serverIpv6Gateway);
    }
    out << "DNS = " << dnsServers.join(", ") << "\n";
  }

  if (!m_junkPacketCount.isEmpty()) {
    out << "Jc = " << m_junkPacketCount << "\n";
  }
  if (!m_junkPacketMinSize.isEmpty()) {
    out << "JMin = " << m_junkPacketMinSize << "\n";
  }
  if (!m_junkPacketMaxSize.isEmpty()) {
    out << "JMax = " << m_junkPacketMaxSize << "\n";
  }
  if (!m_initPacketJunkSize.isEmpty()) {
    out << "S1 = " << m_initPacketJunkSize << "\n";
  }
  if (!m_responsePacketJunkSize.isEmpty()) {
    out << "S2 = " << m_responsePacketJunkSize << "\n";
  }
  if (!m_cookieReplyPacketJunkSize.isEmpty()) {
    out << "S3 = " << m_cookieReplyPacketJunkSize << "\n";
  }
  if (!m_transportPacketJunkSize.isEmpty()) {
    out << "S4 = " << m_transportPacketJunkSize << "\n";
  }
  if (!m_initPacketMagicHeader.isEmpty()) {
    out << "H1 = " << m_initPacketMagicHeader << "\n";
  }
  if (!m_responsePacketMagicHeader.isEmpty()) {
    out << "H2 = " << m_responsePacketMagicHeader << "\n";
  }
  if (!m_underloadPacketMagicHeader.isEmpty()) {
    out << "H3 = " << m_underloadPacketMagicHeader << "\n";
  }
  if (!m_transportPacketMagicHeader.isEmpty()) {
    out << "H4 = " << m_transportPacketMagicHeader << "\n";
  }

  for (const QString& key : m_specialJunk.keys()) {
    if (!m_specialJunk[key].isEmpty()) {
      out << key << " = " << m_specialJunk[key] << "\n";
    }
  }

  if (!m_headerProtectionKey.isEmpty()) {
    out << "HeaderProtectionKey = " << m_headerProtectionKey << "\n";
  }
  if (!m_contentPaddingAddition.isEmpty()) {
    out << "ContentPaddingAddition = " << m_contentPaddingAddition << "\n";
  }
  if (!m_rekeyAfterTime.isEmpty()) {
    out << "RekeyAfterTime = " << m_rekeyAfterTime << "\n";
  }
  if (!m_rekeyTimeout.isEmpty()) {
    out << "RekeyTimeout = " << m_rekeyTimeout << "\n";
  }
  if (!m_rejectAfterTime.isEmpty()) {
    out << "RejectAfterTime = " << m_rejectAfterTime << "\n";
  }
  if (!m_keepaliveTimeout.isEmpty()) {
    out << "KeepaliveTimeout = " << m_keepaliveTimeout << "\n";
  }
  if (!m_maxHandshakeAttempts.isEmpty()) {
    out << "MaxHandshakeAttempts = " << m_maxHandshakeAttempts << "\n";
  }
  if (!m_randomTrailers.isEmpty()) {
    out << "RandomTrailers = " << m_randomTrailers << "\n";
  }
  if (!m_disableCookies.isEmpty()) {
    out << "DisableCookies = " << m_disableCookies << "\n";
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
  if (!m_persistentKeepalive.isEmpty()) {
    out << "PersistentKeepalive = " << m_persistentKeepalive << "\n";
  }

  return content;
}

QString InterfaceConfig::awgBoolToUapi(const QString& value) {
  const QString v = value.trimmed().toLower();
  if (v == QLatin1String("on") || v == QLatin1String("1") ||
      v == QLatin1String("true") || v == QLatin1String("t") ||
      v == QLatin1String("yes")) {
    return QStringLiteral("1");
  }
  return QStringLiteral("0");
}
