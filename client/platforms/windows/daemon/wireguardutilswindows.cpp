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
#include "platforms/windows/windowscommons.h"
#include "windowsdaemon.h"
#include "windowsfirewall.h"

#pragma comment(lib, "iphlpapi.lib")

namespace {
Logger logger("WireguardUtilsWindows");
};  // namespace

WireguardUtilsWindows::WireguardUtilsWindows(QObject* parent)
    : WireguardUtils(parent), m_tunnel(this), m_routeMonitor(this) {
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

  // We don't want to pass a peer just yet, that will happen later with
  // a UAPI command in WireguardUtilsWindows::updatePeer(), so truncate
  // the config file to remove the [Peer] section.
  qsizetype peerStart = configString.indexOf("[Peer]", 0, Qt::CaseSensitive);
  if (peerStart >= 0) {
    configString.truncate(peerStart);
  }

  if (!m_tunnel.start(configString)) {
    logger.error() << "Failed to activate the tunnel service";
    return false;
  }

  // Determine the interface LUID
  NET_LUID luid;
  QString ifAlias = interfaceName();
  DWORD result = ConvertInterfaceAliasToLuid((wchar_t*)ifAlias.utf16(), &luid);
  if (result != 0) {
    logger.error() << "Failed to lookup LUID:" << result;
    return false;
  }
  m_luid = luid.Value;
  m_routeMonitor.setLuid(luid.Value);

  if (config.m_killSwitchEnabled) {
    // Enable the windows firewall
    NET_IFINDEX ifindex;
    ConvertInterfaceLuidToIndex(&luid, &ifindex);
    WindowsFirewall::instance()->enableKillSwitch(ifindex);
  }

  logger.debug() << "Registration completed";
  return true;
}

bool WireguardUtilsWindows::deleteInterface() {
  WindowsFirewall::instance()->disableKillSwitch();
  m_tunnel.stop();
  return true;
}

bool WireguardUtilsWindows::updatePeer(const InterfaceConfig& config) {
  QByteArray publicKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));
  QByteArray pskKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPskKey));

  if (config.m_killSwitchEnabled) {
    // Enable the windows firewall for this peer.
    WindowsFirewall::instance()->enablePeerTraffic(config);
  }
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

  // Exclude the server address, except for multihop exit servers.
  if (config.m_hopType != InterfaceConfig::MultiHopExit) {
    m_routeMonitor.addExclusionRoute(IPAddress(config.m_serverIpv4AddrIn));
    m_routeMonitor.addExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));
  }

  QString reply = m_tunnel.uapiCommand(message);
  logger.debug() << "DATA:" << reply;
  return true;
}

bool WireguardUtilsWindows::deletePeer(const InterfaceConfig& config) {
  QByteArray publicKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));

  // Clear exclustion routes for this peer.
  if (config.m_hopType != InterfaceConfig::MultiHopExit) {
    m_routeMonitor.deleteExclusionRoute(IPAddress(config.m_serverIpv4AddrIn));
    m_routeMonitor.deleteExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));
  }

  // Disable the windows firewall for this peer.
  WindowsFirewall::instance()->disablePeerTraffic(config.m_serverPublicKey);

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
  MIB_IPFORWARD_ROW2 entry;
  buildMibForwardRow(prefix, &entry);

  // Install the route
  DWORD result = CreateIpForwardEntry2(&entry);
  if (result == ERROR_OBJECT_ALREADY_EXISTS) {
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
  return m_routeMonitor.addExclusionRoute(prefix);
}

bool WireguardUtilsWindows::deleteExclusionRoute(const IPAddress& prefix) {
  return m_routeMonitor.deleteExclusionRoute(prefix);
}
