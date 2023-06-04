/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wgquickprocess.h"

#include <QCoreApplication>
#include <QProcess>

#include "logger.h"

namespace {
Logger logger("WgQuickProcess");
}  // namespace

// static
QString WgQuickProcess::createConfigString(
    const InterfaceConfig& config, const QMap<QString, QString>& extra) {
#define VALIDATE(x) \
  if (x.contains("\n")) return "";

  VALIDATE(config.m_privateKey);
  VALIDATE(config.m_deviceIpv4Address);
  VALIDATE(config.m_deviceIpv6Address);
  VALIDATE(config.m_serverIpv4Gateway);
  VALIDATE(config.m_serverIpv6Gateway);
  VALIDATE(config.m_serverPublicKey);
  VALIDATE(config.m_serverIpv4AddrIn);
  VALIDATE(config.m_serverIpv6AddrIn);
#undef VALIDATE

  QString content;
  QTextStream out(&content);
  out << "[Interface]\n";
  out << "PrivateKey = " << config.m_privateKey << "\n";

  QStringList addresses;
  if (!config.m_deviceIpv4Address.isNull()) {
    addresses.append(config.m_deviceIpv4Address);
  }
  if (!config.m_deviceIpv6Address.isNull()) {
    addresses.append(config.m_deviceIpv6Address);
  }
  if (addresses.isEmpty()) {
    logger.error() << "Failed to create WG quick config with no addresses";
    return "";
  }
  out << "Address = " << addresses.join(", ") << "\n";

  if (!config.m_dnsServer.isNull()) {
    QStringList dnsServers(config.m_dnsServer);
    // If the DNS is not the Gateway, it's a user defined DNS
    // thus, not add any other :)
    if (config.m_dnsServer == config.m_serverIpv4Gateway) {
      dnsServers.append(config.m_serverIpv6Gateway);
    }
    out << "DNS = " << dnsServers.join(", ") << "\n";
  }

  // If any extra config was provided, append it now.
  for (const QString& key : extra.keys()) {
    out << key << " = " << extra[key] << "\n";
  }

  out << "\n[Peer]\n";
  out << "PublicKey = " << config.m_serverPublicKey << "\n";
  out << "Endpoint = " << config.m_serverIpv4AddrIn.toUtf8() << ":"
      << config.m_serverPort << "\n";

  /* In theory, we should use the ipv6 endpoint, but wireguard doesn't seem
   * to be happy if there are 2 endpoints.
  out << "Endpoint = [" << config.m_serverIpv6AddrIn << "]:"
      << config.m_serverPort << "\n";
  */
  QStringList ranges;
  for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
    ranges.append(ip.toString());
  }
  out << "AllowedIPs = " << ranges.join(", ") << "\n";

  return content;
}
