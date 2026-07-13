/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wireguardutilswindows.h"

#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2ipdef.h>

#include <QFileInfo>

#include "leakdetector.h"
#include "logger.h"
#include "windowsfirewall.h"

#pragma comment(lib, "iphlpapi.lib")

namespace {
Logger logger("WireguardUtilsWindows");
};  // namespace

std::unique_ptr<WireguardUtilsWindows> WireguardUtilsWindows::create(
    WindowsFirewall* fw, QObject* parent) {
  if (!fw) {
    logger.error() << "WireguardUtilsWindows::create: no wfp handle";
    return {};
  }

  // Can't use make_unique here as the Constructor is private :(
  auto utils = new WireguardUtilsWindows(parent, fw);
  return std::unique_ptr<WireguardUtilsWindows>(utils);
}

WireguardUtilsWindows::WireguardUtilsWindows(QObject* parent, WindowsFirewall* fw)
    : WireguardUtils(parent), m_tunnel(this), m_firewall(fw) {
  MZ_COUNT_CTOR(WireguardUtilsWindows);
  logger.debug() << "WireguardUtilsWindows created.";

  connect(&m_tunnel, &WindowsTunnelService::backendFailure, this,
          [&] { emit backendFailure(); });
}

WireguardUtilsWindows::~WireguardUtilsWindows() {
  MZ_COUNT_DTOR(WireguardUtilsWindows);
  logger.debug() << "WireguardUtilsWindows destroyed.";
}

QList<WireguardUtils::PeerStatus> WireguardUtilsWindows::getPeerStatus() {
  QString reply = m_tunnel.uapiCommand("get=1");
  PeerStatus status;
  QList<PeerStatus> peerList;
  for (const QString& line : reply.split('\n')) {
    int eq = line.indexOf('=');
    if (eq <= 0) {
      continue;
    }
    QString name = line.left(eq);
    QString value = line.mid(eq + 1);

    if (name == "public_key") {
      if (!status.m_pubkey.isEmpty()) {
        peerList.append(status);
      }
      QByteArray pubkey = QByteArray::fromHex(value.toUtf8());
      status = PeerStatus(pubkey.toBase64());
    }

    if (name == "tx_bytes") {
      status.m_txBytes = value.toDouble();
    }
    if (name == "rx_bytes") {
      status.m_rxBytes = value.toDouble();
    }
    if (name == "last_handshake_time_sec") {
      status.m_handshake += value.toLongLong() * 1000;
    }
    if (name == "last_handshake_time_nsec") {
      status.m_handshake += value.toLongLong() / 1000000;
    }
  }
  if (!status.m_pubkey.isEmpty()) {
    peerList.append(status);
  }

  return peerList;
}

bool WireguardUtilsWindows::addInterface(const InterfaceConfig& config) {
  QStringList addresses;
  for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
    addresses.append(ip.toString());
  }

  QMap<QString, QString> extraConfig;
  extraConfig["Table"] = "off";
  QString configString = config.toWgConf(extraConfig);
  if (configString.isEmpty()) {
    logger.error() << "Failed to create a config file";
    return false;
  }

  qsizetype peerStart = configString.indexOf("[Peer]", 0, Qt::CaseSensitive);
  if (peerStart >= 0) {
    configString.truncate(peerStart);
  }

  auto stripLine = [&](const QString& key) {
    qsizetype start = configString.startsWith(key + " = ")
                          ? 0
                          : configString.indexOf("\n" + key + " = ");
    if (start < 0) return;
    if (start != 0) start += 1;
    qsizetype end = configString.indexOf('\n', start);
    if (end < 0) return;
    configString.remove(start, end - start + 1);
  };

  stripLine("DNS");
  if (config.m_deferAddressSetup) {
    // Wintun rejects duplicate IPv4; daemon will assign at swap time.
    stripLine("Address");
  }

  m_ifname = config.m_ifname.isEmpty() ? s_defaultInterfaceName() : config.m_ifname;
  if (!m_tunnel.start(configString, m_ifname)) {
    logger.error() << "Failed to activate the tunnel service";
    return false;
  }

  NET_LUID luid;
  DWORD result = ConvertInterfaceAliasToLuid((wchar_t*)m_ifname.utf16(), &luid);
  if (result != 0) {
    logger.error() << "Failed to lookup LUID:" << result;
    return false;
  }
  m_luid = luid.Value;
  m_routeMonitor = new WindowsRouteMonitor(luid.Value, this);

  logger.debug() << "Registration completed";
  return true;
}

bool WireguardUtilsWindows::deleteInterface() {
  if (m_routeMonitor) {
    m_routeMonitor->deleteLater();
  }

  m_tunnel.stop();
  return true;
}

bool WireguardUtilsWindows::updatePeer(const InterfaceConfig& config) {
  QByteArray publicKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));
  QByteArray pskKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPskKey));

  logger.debug() << "Configuring peer" << publicKey.toHex()
                 << "via" << config.m_serverIpv4AddrIn;

  // Update/create the peer config
  QString message;
  QTextStream out(&message);
  out << "set=1\n";
  out << "public_key=" << QString(publicKey.toHex()) << "\n";
  if (!config.m_serverPskKey.isNull()) {
    out << "preshared_key=" << QString(pskKey.toHex()) << "\n";
  }
  if (!config.m_serverIpv4AddrIn.isNull()) {
    out << "endpoint=" << config.m_serverIpv4AddrIn << ":";
  } else if (!config.m_serverIpv6AddrIn.isNull()) {
    out << "endpoint=[" << config.m_serverIpv6AddrIn << "]:";
  } else {
    logger.warning() << "Failed to create peer with no endpoints";
    return false;
  }
  out << config.m_serverPort << "\n";

  out << "replace_allowed_ips=true\n";
  out << "persistent_keepalive_interval=" << WG_KEEPALIVE_PERIOD << "\n";
  for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
    out << "allowed_ip=" << ip.toString() << "\n";
  }

  QString reply = m_tunnel.uapiCommand(message);
  logger.debug() << "DATA:" << reply;
  return true;
}

bool WireguardUtilsWindows::deletePeer(const InterfaceConfig& config) {
  QByteArray publicKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));

  QString message;
  QTextStream out(&message);
  out << "set=1\n";
  out << "public_key=" << QString(publicKey.toHex()) << "\n";
  out << "remove=true\n";

  QString reply = m_tunnel.uapiCommand(message);
  logger.debug() << "DATA:" << reply;
  return true;
}

void WireguardUtilsWindows::buildMibForwardRow(const IPAddress& prefix,
                                               void* row) {
  MIB_IPFORWARD_ROW2* entry = (MIB_IPFORWARD_ROW2*)row;
  InitializeIpForwardEntry(entry);

  // Populate the next hop
  if (prefix.type() == QAbstractSocket::IPv6Protocol) {
    InetPtonA(AF_INET6, qPrintable(prefix.address().toString()),
              &entry->DestinationPrefix.Prefix.Ipv6.sin6_addr);
    entry->DestinationPrefix.Prefix.Ipv6.sin6_family = AF_INET6;
    entry->DestinationPrefix.PrefixLength = prefix.prefixLength();
  } else {
    InetPtonA(AF_INET, qPrintable(prefix.address().toString()),
              &entry->DestinationPrefix.Prefix.Ipv4.sin_addr);
    entry->DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
    entry->DestinationPrefix.PrefixLength = prefix.prefixLength();
  }
  entry->InterfaceLuid.Value = m_luid;
  entry->NextHop.si_family = entry->DestinationPrefix.Prefix.si_family;

  // Set the rest of the flags for a static route.
  entry->ValidLifetime = 0xffffffff;
  entry->PreferredLifetime = 0xffffffff;
  entry->Metric = 0;
  entry->Protocol = MIB_IPPROTO_NETMGMT;
  entry->Loopback = false;
  entry->AutoconfigureAddress = false;
  entry->Publish = false;
  entry->Immortal = false;
  entry->Age = 0;
}

bool WireguardUtilsWindows::updateRoutePrefix(const IPAddress& prefix) {
  if (m_routeMonitor && (prefix.prefixLength() == 0)) {
    // If we are setting up a default route, instruct the route monitor to
    // capture traffic to all non-excluded destinations
    m_routeMonitor->setDetaultRouteCapture(true);
  }
  // Build the route
  
  MIB_IPFORWARD_ROW2 entry;
  buildMibForwardRow(prefix, &entry);

  // Install the route
  DWORD result = CreateIpForwardEntry2(&entry);
  if (result == ERROR_OBJECT_ALREADY_EXISTS) {
    return true;
  }

  // Case for ipv6 route with disabled ipv6
  if (prefix.address().protocol() == QAbstractSocket::IPv6Protocol
      && result == ERROR_NOT_FOUND) {
    return true;
  }

  if (result != NO_ERROR) {
    logger.error() << "Failed to create route to"
                   << prefix.toString()
                   << "result:" << result;
  }
  return result == NO_ERROR;
}

bool WireguardUtilsWindows::deleteRoutePrefix(const IPAddress& prefix) {
  if (m_routeMonitor && (prefix.prefixLength() == 0)) {
    // Deactivate the route capture feature.
    m_routeMonitor->setDetaultRouteCapture(false);
  }
  // Build the route
  
  MIB_IPFORWARD_ROW2 entry;
  buildMibForwardRow(prefix, &entry);

  // Install the route
  DWORD result = DeleteIpForwardEntry2(&entry);
  if (result == ERROR_NOT_FOUND) {
    return true;
  }
  if (result != NO_ERROR) {
    logger.error() << "Failed to delete route to"
                   << prefix.toString()
                   << "result:" << result;
  }
  return result == NO_ERROR;
}

bool WireguardUtilsWindows::addExclusionRoute(const IPAddress& prefix) {
  return m_routeMonitor->addExclusionRoute(prefix);
}

bool WireguardUtilsWindows::deleteExclusionRoute(const IPAddress& prefix) {
  return m_routeMonitor->deleteExclusionRoute(prefix);
}

bool WireguardUtilsWindows::excludeLocalNetworks(
    const QList<IPAddress>& addresses) {
  // If the interface isn't up then something went horribly wrong.
  Q_ASSERT(m_routeMonitor);
  // For each destination - attempt to exclude it from the VPN tunnel.
  bool result = true;
  for (const IPAddress& prefix : addresses) {
    if (!m_routeMonitor->addExclusionRoute(prefix)) {
      result = false;
    }
  }
  // Permit LAN traffic through the firewall.
  if (!m_firewall->enableLanBypass(addresses)) {
    result = false;
  }

  return result;
}
