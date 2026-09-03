/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wireguardutilswindows.h"

#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#include <winsvc.h>
#include <ws2ipdef.h>

#include <QFileInfo>

#include "leakdetector.h"
#include "logger.h"
#include "windowsfirewall.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace {
Logger logger("WireguardUtilsWindows");

// Suspend wcmSvc for bulk route installs to avoid per-route stalls, matching
constexpr int kWcmSuspendThreshold = 500;

LONG (NTAPI* g_NtSuspendProcess)(HANDLE ProcessHandle) = nullptr;
LONG (NTAPI* g_NtResumeProcess)(HANDLE ProcessHandle) = nullptr;

#ifndef WG_STATUS_SUCCESS
#define WG_STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

bool ensureNtFunctions() {
  static bool initialized = false;
  static bool ok = false;
  if (initialized) {
    return ok;
  }
  initialized = true;

  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!ntdll) {
    return false;
  }
  g_NtSuspendProcess = reinterpret_cast<decltype(g_NtSuspendProcess)>(
      GetProcAddress(ntdll, "NtSuspendProcess"));
  g_NtResumeProcess = reinterpret_cast<decltype(g_NtResumeProcess)>(
      GetProcAddress(ntdll, "NtResumeProcess"));
  ok = (g_NtSuspendProcess != nullptr) && (g_NtResumeProcess != nullptr);
  return ok;
}

bool ensureDebugPrivilege() {
  static bool tried = false;
  static bool ok = false;
  if (tried) {
    return ok;
  }
  tried = true;

  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
    return false;
  }
  TOKEN_PRIVILEGES priv = {};
  if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME,
                             &priv.Privileges[0].Luid)) {
    CloseHandle(token);
    return false;
  }
  priv.PrivilegeCount = 1;
  priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  ok = AdjustTokenPrivileges(token, FALSE, &priv, sizeof(priv), nullptr,
                             nullptr) != 0;
  CloseHandle(token);
  return ok;
}

DWORD getServicePid(LPCWSTR serviceName) {
  SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return 0;
  }
  SC_HANDLE svc = OpenServiceW(scm, serviceName, SERVICE_QUERY_STATUS);
  if (!svc) {
    CloseServiceHandle(scm);
    return 0;
  }
  SERVICE_STATUS_PROCESS ssp = {};
  DWORD bytesNeeded = 0;
  QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                       reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp),
                       &bytesNeeded);
  CloseServiceHandle(svc);
  CloseServiceHandle(scm);
  return ssp.dwProcessId;
}

class WcmSuspender {
 public:
  explicit WcmSuspender(int pendingRoutes) {
    if (pendingRoutes < kWcmSuspendThreshold) {
      return;
    }
    if (!ensureNtFunctions()) {
      return;
    }
    ensureDebugPrivilege();

    DWORD pid = getServicePid(L"wcmSvc");
    if (pid == 0) {
      return;
    }
    m_handle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!m_handle) {
      return;
    }
    if (g_NtSuspendProcess(m_handle) == WG_STATUS_SUCCESS) {
      m_suspended = true;
      logger.debug() << "wcmSvc suspended for bulk route install"
                     << "(pending:" << pendingRoutes << ")";
    } else {
      CloseHandle(m_handle);
      m_handle = nullptr;
    }
  }
  ~WcmSuspender() {
    if (m_handle) {
      if (m_suspended) {
        g_NtResumeProcess(m_handle);
        logger.debug() << "wcmSvc resumed";
      }
      CloseHandle(m_handle);
    }
  }
  WcmSuspender(const WcmSuspender&) = delete;
  WcmSuspender& operator=(const WcmSuspender&) = delete;

 private:
  HANDLE m_handle = nullptr;
  bool m_suspended = false;
};

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
  m_routeMonitor = new WindowsRouteMonitor(luid.Value, this);

  if (config.m_killSwitchEnabled) {
    // Enable the windows firewall
    NET_IFINDEX ifindex;
    ConvertInterfaceLuidToIndex(&luid, &ifindex);
    m_firewall->allowAllTraffic();
    m_firewall->enableInterface(ifindex);
  }

  logger.debug() << "Registration completed";
  return true;
}

bool WireguardUtilsWindows::deleteInterface() {
  if (m_routeMonitor) {
    m_routeMonitor->deleteLater();
  }

  m_firewall->disableKillSwitch();
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
    m_firewall->enablePeerTraffic(config);
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
  if (!config.m_persistentKeepalive.isEmpty()) {
    out << "persistent_keepalive_interval=" << config.m_persistentKeepalive << "\n";
  }
  for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
    out << "allowed_ip=" << ip.toString() << "\n";
  }

  // Exclude the server address, except for multihop exit servers.
  if (m_routeMonitor && config.m_hopType != InterfaceConfig::MultiHopExit) {
    m_routeMonitor->addExclusionRoute(IPAddress(config.m_serverIpv4AddrIn));
    m_routeMonitor->addExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));
  }

  QString reply = m_tunnel.uapiCommand(message);
  logger.debug() << "DATA:" << reply;
  return true;
}

bool WireguardUtilsWindows::deletePeer(const InterfaceConfig& config) {
  QByteArray publicKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));

  // Clear exclustion routes for this peer.
  if (m_routeMonitor && config.m_hopType != InterfaceConfig::MultiHopExit) {
    m_routeMonitor->deleteExclusionRoute(IPAddress(config.m_serverIpv4AddrIn));
    m_routeMonitor->deleteExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));
  }

  // Disable the windows firewall for this peer.
  m_firewall->disablePeerTraffic(config.m_serverPublicKey);

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

bool WireguardUtilsWindows::updateRoutePrefixes(
    const QList<IPAddress>& prefixes) {
  if (prefixes.isEmpty()) {
    return true;
  }

  if (m_routeMonitor) {
    for (const IPAddress& prefix : prefixes) {
      if (prefix.prefixLength() == 0) {
        m_routeMonitor->setDetaultRouteCapture(true);
        break;
      }
    }
  }

  WcmSuspender suspender(prefixes.size());

  bool ok = true;
  for (const IPAddress& prefix : prefixes) {
    MIB_IPFORWARD_ROW2 entry;
    buildMibForwardRow(prefix, &entry);
    DWORD result = CreateIpForwardEntry2(&entry);
    if (result == ERROR_OBJECT_ALREADY_EXISTS) {
      continue;
    }
    // IPv6 might be disabled on the host.
    if (prefix.address().protocol() == QAbstractSocket::IPv6Protocol &&
        result == ERROR_NOT_FOUND) {
      continue;
    }
    if (result != NO_ERROR) {
      logger.error() << "Failed to create route to" << prefix.toString()
                     << "result:" << result;
      ok = false;
    }
  }
  return ok;
}

bool WireguardUtilsWindows::addExclusionRoute(const IPAddress& prefix) {
  return m_routeMonitor->addExclusionRoute(prefix);
}

bool WireguardUtilsWindows::addExclusionRoutes(
    const QList<IPAddress>& prefixes) {
  if (!m_routeMonitor || prefixes.isEmpty()) {
    return true;
  }
  WcmSuspender suspender(prefixes.size());
  m_routeMonitor->addExclusionRoutes(prefixes);
  return true;
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
