/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsfirewall.h"

#include <comdef.h>
#include <fwpmu.h>
#include <guiddef.h>
#include <initguid.h>
#include <netfw.h>
#include <qaccessible.h>
#include <qassert.h>
#include <stdio.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include "winsock.h"

#include <QApplication>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QObject>
#include <QScopeGuard>
#include <QtEndian>

#include "ipaddress.h"
#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/windowsutils.h"

#define IPV6_ADDRESS_SIZE 16

// ID for the Firewall Sublayer
DEFINE_GUID(ST_FW_WINFW_BASELINE_SUBLAYER_KEY, 0xc78056ff, 0x2bc1, 0x4211, 0xaa,
            0xdd, 0x7f, 0x35, 0x8d, 0xef, 0x20, 0x2d);
// ID for the Mullvad Split-Tunnel Sublayer Provider
DEFINE_GUID(ST_FW_PROVIDER_KEY, 0xe2c114ee, 0xf32a, 0x4264, 0xa6, 0xcb, 0x3f,
            0xa7, 0x99, 0x63, 0x56, 0xd9);

namespace {
Logger logger("WindowsFirewall");
WindowsFirewall* s_instance = nullptr;

// Note Filter Weight may be between 0-15!
constexpr uint8_t LOW_WEIGHT = 0;
constexpr uint8_t MED_WEIGHT = 7;
constexpr uint8_t HIGH_WEIGHT = 13;
constexpr uint8_t MAX_WEIGHT = 15;
}  // namespace

WindowsFirewall* WindowsFirewall::create(QObject* parent) {
  if (s_instance != nullptr) {
    // Only one instance of the firewall is allowed
//    Q_ASSERT(false);
    return s_instance;
  }
  HANDLE engineHandle = nullptr;
  DWORD result = ERROR_SUCCESS;
  // Use dynamic sessions for efficiency and safety:
  //  -> Filtering policy objects are deleted even when the application crashes/
  //  deamon goes down
  FWPM_SESSION0 session;
  memset(&session, 0, sizeof(session));
  session.flags = FWPM_SESSION_FLAG_DYNAMIC;

  logger.debug() << "Opening the filter engine.";

  result = FwpmEngineOpen0(nullptr, RPC_C_AUTHN_WINNT, nullptr, &session,
                           &engineHandle);

  if (result != ERROR_SUCCESS) {
    WindowsUtils::windowsLog("FwpmEngineOpen0 failed");
    return nullptr;
  }
  logger.debug() << "Filter engine opened successfully.";
  if (!initSublayer()) {
    return nullptr;
  }
  s_instance = new WindowsFirewall(engineHandle, parent);
  return s_instance;
}

WindowsFirewall::WindowsFirewall(HANDLE session, QObject* parent)
    : QObject(parent), m_sessionHandle(session) {
  MZ_COUNT_CTOR(WindowsFirewall);
}

WindowsFirewall::~WindowsFirewall() {
  MZ_COUNT_DTOR(WindowsFirewall);
  if (m_sessionHandle != INVALID_HANDLE_VALUE) {
    CloseHandle(m_sessionHandle);
  }
}

// static
bool WindowsFirewall::initSublayer() {
  // If we were not able to aquire a handle, this will fail anyway.
  // We need to open up another handle because of wfp rules:
  // If a wfp resource was created with SESSION_DYNAMIC,
  // the session exlusively owns the resource, meaning the driver can't add
  // filters to the sublayer. So let's have non dynamic session only for the
  // sublayer creation. This means the Layer exists until the next Reboot.
  DWORD result = ERROR_SUCCESS;
  HANDLE wfp = INVALID_HANDLE_VALUE;
  FWPM_SESSION0 session;
  memset(&session, 0, sizeof(session));

  logger.debug() << "Opening the filter engine";
  result = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &wfp);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmEngineOpen0 failed. Return value:.\n" << result;
    return false;
  }
  auto cleanup = qScopeGuard([&] { FwpmEngineClose0(wfp); });

  // Check if the Layer Already Exists
  FWPM_SUBLAYER0* maybeLayer;
  result = FwpmSubLayerGetByKey0(wfp, &ST_FW_WINFW_BASELINE_SUBLAYER_KEY,
                                 &maybeLayer);
  if (result == ERROR_SUCCESS) {
    logger.debug() << "The Sublayer Already Exists!";
    FwpmFreeMemory0((void**)&maybeLayer);
    return true;
  }

  // Step 1: Start Transaction
  result = FwpmTransactionBegin(wfp, NULL);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionBegin0 failed. Return value:.\n"
                   << result;
    return false;
  }

  // Step 3: Add Sublayer
  FWPM_SUBLAYER0 subLayer;
  memset(&subLayer, 0, sizeof(subLayer));
  subLayer.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
  subLayer.displayData.name = (PWSTR)L"Amnezia-SplitTunnel-Sublayer";
  subLayer.displayData.description =
      (PWSTR)L"Filters that enforce a good baseline";
  subLayer.weight = 0xFFFF;

  result = FwpmSubLayerAdd0(wfp, &subLayer, NULL);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmSubLayerAdd0 failed. Return value:.\n" << result;
    return false;
  }
  // Step 4: Commit!
  result = FwpmTransactionCommit0(wfp);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionCommit0 failed. Return value:.\n"
                   << result;
    return false;
  }
  logger.debug() << "Initialised Sublayer";
  return true;
}

bool WindowsFirewall::enableInterface(int vpnAdapterIndex) {
// Checks if the FW_Rule was enabled succesfully,
// disables the whole killswitch and returns false if not.
#define FW_OK(rule)                                                       \
  {                                                                       \
    auto result = FwpmTransactionBegin(m_sessionHandle, NULL);            \
    if (result != ERROR_SUCCESS) {                                        \
      disableKillSwitch();                                                \
      return false;                                                       \
    }                                                                     \
    if (!rule) {                                                          \
      FwpmTransactionAbort0(m_sessionHandle);                             \
      disableKillSwitch();                                                \
      return false;                                                       \
    }                                                                     \
    result = FwpmTransactionCommit0(m_sessionHandle);                     \
    if (result != ERROR_SUCCESS) {                                        \
      logger.error() << "FwpmTransactionCommit0 failed. Return value:.\n" \
                     << result;                                           \
      return false;                                                       \
    }                                                                     \
  }

  logger.info() << "Enabling firewall Using Adapter:" << vpnAdapterIndex;
  FW_OK(allowTrafficOfAdapter(vpnAdapterIndex, MED_WEIGHT,
                              "Allow usage of VPN Adapter"));
  FW_OK(allowDHCPTraffic(MED_WEIGHT, "Allow DHCP Traffic"));
  FW_OK(allowHyperVTraffic(MED_WEIGHT, "Allow Hyper-V Traffic"));
  FW_OK(allowTrafficForAppOnAll(getCurrentPath(), MAX_WEIGHT,
                                "Allow all for AmneziaVPN.exe"));
  FW_OK(blockTrafficOnPort(53, MED_WEIGHT, "Block all DNS"));
  FW_OK(
      allowLoopbackTraffic(MED_WEIGHT, "Allow Loopback traffic on device %1"));

  logger.debug() << "Killswitch on! Rules:" << m_activeRules.length();
  return true;
#undef FW_OK
}

// Allow unprotected traffic sent to the following local address ranges.
bool WindowsFirewall::enableLanBypass(const QList<IPAddress>& ranges) {
  // Start the firewall transaction
  auto result = FwpmTransactionBegin(m_sessionHandle, NULL);
  if (result != ERROR_SUCCESS) {
    disableKillSwitch();
    return false;
  }
  auto cleanup = qScopeGuard([&] {
    FwpmTransactionAbort0(m_sessionHandle);
    disableKillSwitch();
  });

  // Blocking unprotected traffic
  for (const IPAddress& prefix : ranges) {
    if (!allowTrafficTo(prefix, LOW_WEIGHT + 1, "Allow LAN bypass traffic")) {
      return false;
    }
  }

  result = FwpmTransactionCommit0(m_sessionHandle);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionCommit0 failed with error:" << result;
    return false;
  }

  cleanup.dismiss();
  return true;
}

bool WindowsFirewall::enablePeerTraffic(const InterfaceConfig& config) {
  // Start the firewall transaction
  auto result = FwpmTransactionBegin(m_sessionHandle, NULL);
  if (result != ERROR_SUCCESS) {
    disableKillSwitch();
    return false;
  }
  auto cleanup = qScopeGuard([&] {
    FwpmTransactionAbort0(m_sessionHandle);
    disableKillSwitch();
  });

  // Build the firewall rules for this peer.
  logger.info() << "Enabling traffic for peer"
                << config.m_serverPublicKey;
  if (!blockTrafficTo(config.m_allowedIPAddressRanges, LOW_WEIGHT,
                      "Block Internet", config.m_serverPublicKey)) {
    return false;
  }
  if (!config.m_dnsServer.isEmpty()) {
    if (!allowTrafficTo(QHostAddress(config.m_dnsServer), 53, HIGH_WEIGHT,
                        "Allow DNS-Server", config.m_serverPublicKey)) {
      return false;
    }
    // In some cases, we might configure a 2nd DNS server for IPv6, however
    // this should probably be cleaned up by converting m_dnsServer into
    // a QStringList instead.
    if (config.m_dnsServer == config.m_serverIpv4Gateway) {
      if (!allowTrafficTo(QHostAddress(config.m_serverIpv6Gateway), 53,
                          HIGH_WEIGHT, "Allow extra IPv6 DNS-Server",
                          config.m_serverPublicKey)) {
        return false;
      }
    }
  }

  if (!config.m_excludedAddresses.empty()) {
    for (const QString& i : config.m_excludedAddresses) {
      logger.debug() << "excludedAddresses range: " << i;

      if (!allowTrafficTo(i, HIGH_WEIGHT,
                               "Allow Ecxlude route", config.m_serverPublicKey)) {
        return false;
      }
    }
  }

  result = FwpmTransactionCommit0(m_sessionHandle);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionCommit0 failed with error:" << result;
    return false;
  }

  cleanup.dismiss();
  return true;
}

bool WindowsFirewall::disablePeerTraffic(const QString& pubkey) {
  auto result = FwpmTransactionBegin(m_sessionHandle, NULL);
  auto cleanup = qScopeGuard([&] {
    if (result != ERROR_SUCCESS) {
      FwpmTransactionAbort0(m_sessionHandle);
    }
  });
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionBegin0 failed. Return value:.\n"
                   << result;
    return false;
  }

  logger.info() << "Disabling traffic for peer" << pubkey;
  for (const auto& filterID : m_peerRules.values(pubkey)) {
    FwpmFilterDeleteById0(m_sessionHandle, filterID);
    m_peerRules.remove(pubkey, filterID);
  }

  // Commit!
  result = FwpmTransactionCommit0(m_sessionHandle);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionCommit0 failed. Return value:.\n"
                   << result;
    return false;
  }
  return true;
}

bool WindowsFirewall::disableKillSwitch() {
  auto result = FwpmTransactionBegin(m_sessionHandle, NULL);
  auto cleanup = qScopeGuard([&] {
    if (result != ERROR_SUCCESS) {
      FwpmTransactionAbort0(m_sessionHandle);
    }
  });
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionBegin0 failed. Return value:.\n"
                   << result;
    return false;
  }

  for (const auto& filterID : m_peerRules.values()) {
    FwpmFilterDeleteById0(m_sessionHandle, filterID);
  }

  for (const auto& filterID : qAsConst(m_activeRules)) {
    FwpmFilterDeleteById0(m_sessionHandle, filterID);
  }

  // Commit!
  result = FwpmTransactionCommit0(m_sessionHandle);
  if (result != ERROR_SUCCESS) {
    logger.error() << "FwpmTransactionCommit0 failed. Return value:.\n"
                   << result;
    return false;
  }
  m_peerRules.clear();
  m_activeRules.clear();
  logger.debug() << "Firewall Disabled!";
  return true;
}

bool WindowsFirewall::allowTrafficForAppOnAll(const QString& exePath,
                                              int weight,
                                              const QString& title) {
  DWORD result = ERROR_SUCCESS;
  Q_ASSERT(weight <= 15);

  // Get the AppID for the Executable;
  QString appName = QFileInfo(exePath).baseName();
  std::wstring wstr = exePath.toStdWString();
  PCWSTR appPath = wstr.c_str();
  FWP_BYTE_BLOB* appID = NULL;
  result = FwpmGetAppIdFromFileName0(appPath, &appID);
  if (result != ERROR_SUCCESS) {
    WindowsUtils::windowsLog("FwpmGetAppIdFromFileName0 failure");
    return false;
  }
  // Condition: Request must come from the .exe
  FWPM_FILTER_CONDITION0 conds;
  conds.fieldKey = FWPM_CONDITION_ALE_APP_ID;
  conds.matchType = FWP_MATCH_EQUAL;
  conds.conditionValue.type = FWP_BYTE_BLOB_TYPE;
  conds.conditionValue.byteBlob = appID;

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.filterCondition = &conds;
  filter.numFilterConditions = 1;
  filter.action.type = FWP_ACTION_PERMIT;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
  filter.flags = FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;  // Make this decision
                                                       // only blockable by veto
  // Build and add the Filters
  // #1 Permit outbound IPv4 traffic.
  {
    QString desc("Permit (out) IPv4 Traffic of: " + appName);
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    if (!enableFilter(&filter, title, desc)) {
      return false;
    }
  }
  // #2 Permit inbound IPv4 traffic.
  {
    QString desc("Permit (in) IPv4 Traffic of: " + appName);
    filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
    if (!enableFilter(&filter, title, desc)) {
      return false;
    }
  }
  return true;
}

bool WindowsFirewall::allowTrafficOfAdapter(int networkAdapter, uint8_t weight,
                                            const QString& title) {
  FWPM_FILTER_CONDITION0 conds;
  // Condition: Request must be targeting the TUN interface
  conds.fieldKey = FWPM_CONDITION_INTERFACE_INDEX;
  conds.matchType = FWP_MATCH_EQUAL;
  conds.conditionValue.type = FWP_UINT32;
  conds.conditionValue.uint32 = networkAdapter;

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.filterCondition = &conds;
  filter.numFilterConditions = 1;
  filter.action.type = FWP_ACTION_PERMIT;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;

  QString description("Allow %0 traffic on Adapter %1");
  // #1 Permit outbound IPv4 traffic.
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  if (!enableFilter(&filter, title,
                    description.arg("out").arg(networkAdapter))) {
    return false;
  }
  // #2 Permit inbound IPv4 traffic.
  filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
  if (!enableFilter(&filter, title,
                    description.arg("in").arg(networkAdapter))) {
    return false;
  }
  // #3 Permit outbound IPv6 traffic.
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
  if (!enableFilter(&filter, title,
                    description.arg("out").arg(networkAdapter))) {
    return false;
  }
  // #4 Permit inbound IPv6 traffic.
  filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
  if (!enableFilter(&filter, title,
                    description.arg("in").arg(networkAdapter))) {
    return false;
  }
  return true;
}

bool WindowsFirewall::allowTrafficTo(const IPAddress& addr, int weight,
                                     const QString& title,
                                     const QString& peer) {
  GUID layerKeyOut;
  GUID layerKeyIn;
  if (addr.type() == QAbstractSocket::IPv4Protocol) {
    layerKeyOut = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    layerKeyIn = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
  } else {
    layerKeyOut = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
    layerKeyIn = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
  }

  // Match the IP address range.
  FWPM_FILTER_CONDITION0 cond[1] = {};
  FWP_RANGE0 ipRange;
  QByteArray lowIpV6Buffer;
  QByteArray highIpV6Buffer;

  importAddress(addr.address(), ipRange.valueLow, &lowIpV6Buffer);
  importAddress(addr.broadcastAddress(), ipRange.valueHigh, &highIpV6Buffer);

  cond[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
  cond[0].matchType = FWP_MATCH_RANGE;
  cond[0].conditionValue.type = FWP_RANGE_TYPE;
  cond[0].conditionValue.rangeValue = &ipRange;

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.action.type = FWP_ACTION_PERMIT;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
  filter.numFilterConditions = 1;
  filter.filterCondition = cond;

  // Send the filters down to the firewall.
  QString description = "Permit traffic %1 " + addr.toString();
  filter.layerKey = layerKeyOut;
  if (!enableFilter(&filter, title, description.arg("to"), peer)) {
    return false;
  }
  filter.layerKey = layerKeyIn;
  if (!enableFilter(&filter, title, description.arg("from"), peer)) {
    return false;
  }
  return true;
}

bool WindowsFirewall::allowTrafficTo(const QHostAddress& targetIP, uint port,
                                     int weight, const QString& title,
                                     const QString& peer) {
  bool isIPv4 = targetIP.protocol() == QAbstractSocket::IPv4Protocol;
  GUID layerOut =
      isIPv4 ? FWPM_LAYER_ALE_AUTH_CONNECT_V4 : FWPM_LAYER_ALE_AUTH_CONNECT_V6;
  GUID layerIn = isIPv4 ? FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4
                        : FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;

  quint32_be ipBigEndian;
  quint32 ip = targetIP.toIPv4Address();
  qToBigEndian(ip, &ipBigEndian);

  // Allow Traffic to IP with PORT using any protocol
  FWPM_FILTER_CONDITION0 conds[4];
  conds[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
  conds[0].matchType = FWP_MATCH_EQUAL;
  conds[0].conditionValue.type = FWP_UINT8;
  conds[0].conditionValue.uint8 = (IPPROTO_UDP);

  conds[1].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
  conds[1].matchType = FWP_MATCH_EQUAL;
  conds[1].conditionValue.type = FWP_UINT8;
  conds[1].conditionValue.uint16 = (IPPROTO_TCP);

  conds[2].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
  conds[2].matchType = FWP_MATCH_EQUAL;
  conds[2].conditionValue.type = FWP_UINT16;
  conds[2].conditionValue.uint16 = port;

  conds[3].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
  conds[3].matchType = FWP_MATCH_EQUAL;
  QByteArray buffer;
  // will hold v6 Addess bytes if present
  importAddress(targetIP, conds[3].conditionValue, &buffer);

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.filterCondition = conds;
  filter.numFilterConditions = 4;
  filter.action.type = FWP_ACTION_PERMIT;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
  filter.flags = FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT;  // Hard Permit!

  QString description("Permit traffic %1 %2 on port %3");
  filter.layerKey = layerOut;
  if (!enableFilter(&filter, title,
                    description.arg("to").arg(targetIP.toString()).arg(port),
                    peer)) {
    return false;
  }
  filter.layerKey = layerIn;
  if (!enableFilter(&filter, title,
                    description.arg("from").arg(targetIP.toString()).arg(port),
                    peer)) {
    return false;
  }
  return true;
}

bool WindowsFirewall::allowDHCPTraffic(uint8_t weight, const QString& title) {
  // Allow outbound DHCPv4
  {
    FWPM_FILTER_CONDITION0 conds[4];
    // Condition: Request must be targeting the TUN interface
    conds[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
    conds[0].matchType = FWP_MATCH_EQUAL;
    conds[0].conditionValue.type = FWP_UINT8;
    conds[0].conditionValue.uint8 = (IPPROTO_UDP);

    conds[1].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
    conds[1].matchType = FWP_MATCH_EQUAL;
    conds[1].conditionValue.type = FWP_UINT16;
    conds[1].conditionValue.uint16 = (68);

    conds[2].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
    conds[2].matchType = FWP_MATCH_EQUAL;
    conds[2].conditionValue.type = FWP_UINT16;
    conds[2].conditionValue.uint16 = 67;

    conds[3].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    conds[3].matchType = FWP_MATCH_EQUAL;
    conds[3].conditionValue.type = FWP_UINT32;
    conds[3].conditionValue.uint32 = (0xffffffff);

    // Assemble the Filter base
    FWPM_FILTER0 filter;
    memset(&filter, 0, sizeof(filter));
    filter.filterCondition = conds;
    filter.numFilterConditions = 4;
    filter.action.type = FWP_ACTION_PERMIT;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = weight;
    filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;

    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;

    if (!enableFilter(&filter, title, "Allow Outbound DHCP")) {
      return false;
    }
  }
  // Allow inbound DHCPv4
  {
    FWPM_FILTER_CONDITION0 conds[3];
    conds[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
    conds[0].matchType = FWP_MATCH_EQUAL;
    conds[0].conditionValue.type = FWP_UINT8;
    conds[0].conditionValue.uint8 = (IPPROTO_UDP);

    conds[1].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
    conds[1].matchType = FWP_MATCH_EQUAL;
    conds[1].conditionValue.type = FWP_UINT16;
    conds[1].conditionValue.uint16 = (68);

    conds[2].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
    conds[2].matchType = FWP_MATCH_EQUAL;
    conds[2].conditionValue.type = FWP_UINT16;
    conds[2].conditionValue.uint16 = 67;

    // Assemble the Filter base
    FWPM_FILTER0 filter;
    memset(&filter, 0, sizeof(filter));
    filter.filterCondition = conds;
    filter.numFilterConditions = 3;
    filter.action.type = FWP_ACTION_PERMIT;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = weight;
    filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;

    if (!enableFilter(&filter, title, "Allow inbound DHCP")) {
      return false;
    }
  }

  // Allow outbound DHCPv6
  {
    FWPM_FILTER_CONDITION0 conds[3];
    // Condition: Request must be targeting the TUN interface
    conds[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
    conds[0].matchType = FWP_MATCH_EQUAL;
    conds[0].conditionValue.type = FWP_UINT8;
    conds[0].conditionValue.uint8 = (IPPROTO_UDP);

    conds[1].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
    conds[1].matchType = FWP_MATCH_EQUAL;
    conds[1].conditionValue.type = FWP_UINT16;
    conds[1].conditionValue.uint16 = (68);

    conds[2].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
    conds[2].matchType = FWP_MATCH_EQUAL;
    conds[2].conditionValue.type = FWP_UINT16;
    conds[2].conditionValue.uint16 = 67;

    // Assemble the Filter base
    FWPM_FILTER0 filter;
    memset(&filter, 0, sizeof(filter));
    filter.filterCondition = conds;
    filter.numFilterConditions = 3;
    filter.action.type = FWP_ACTION_PERMIT;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = weight;
    filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;

    if (!enableFilter(&filter, title, "Allow outbound DHCPv6")) {
      return false;
    }
  }

  // Allow inbound DHCPv6
  {
    FWPM_FILTER_CONDITION0 conds[3];
    conds[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
    conds[0].matchType = FWP_MATCH_EQUAL;
    conds[0].conditionValue.type = FWP_UINT8;
    conds[0].conditionValue.uint8 = (IPPROTO_UDP);

    conds[1].fieldKey = FWPM_CONDITION_IP_LOCAL_PORT;
    conds[1].matchType = FWP_MATCH_EQUAL;
    conds[1].conditionValue.type = FWP_UINT16;
    conds[1].conditionValue.uint16 = (68);

    conds[2].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
    conds[2].matchType = FWP_MATCH_EQUAL;
    conds[2].conditionValue.type = FWP_UINT16;
    conds[2].conditionValue.uint16 = 67;

    // Assemble the Filter base
    FWPM_FILTER0 filter;
    memset(&filter, 0, sizeof(filter));
    filter.filterCondition = conds;
    filter.numFilterConditions = 3;
    filter.action.type = FWP_ACTION_PERMIT;
    filter.weight.type = FWP_UINT8;
    filter.weight.uint8 = weight;
    filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;
    filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
    if (!enableFilter(&filter, title, "Allow inbound DHCPv6")) {
      return false;
    }
  }
  return true;
}

// Allows the internal Hyper-V Switches to work.
bool WindowsFirewall::allowHyperVTraffic(uint8_t weight, const QString& title) {
  FWPM_FILTER_CONDITION0 cond;
  // Condition: Request must be targeting the TUN interface
  cond.fieldKey = FWPM_CONDITION_L2_FLAGS;
  cond.matchType = FWP_MATCH_EQUAL;
  cond.conditionValue.type = FWP_UINT32;
  cond.conditionValue.uint32 = FWP_CONDITION_L2_IS_VM2VM;

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.filterCondition = &cond;
  filter.numFilterConditions = 1;
  filter.action.type = FWP_ACTION_PERMIT;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;

  // #1 Permit Hyper-V => Hyper-V outbound.
  filter.layerKey = FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE;
  if (!enableFilter(&filter, title, "Permit Hyper-V => Hyper-V outbound")) {
    return false;
  }
  // #2 Permit Hyper-V => Hyper-V inbound.
  filter.layerKey = FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE;
  if (!enableFilter(&filter, title, "Permit Hyper-V => Hyper-V inbound")) {
    return false;
  }
  return true;
}

bool WindowsFirewall::blockTrafficTo(const IPAddress& addr, uint8_t weight,
                                     const QString& title,
                                     const QString& peer) {
  QString description("Block traffic %1 %2 ");

  auto lower = addr.address();
  auto upper = addr.broadcastAddress();

  const bool isV4 = addr.type() == QAbstractSocket::IPv4Protocol;
  const GUID layerKeyOut =
      isV4 ? FWPM_LAYER_ALE_AUTH_CONNECT_V4 : FWPM_LAYER_ALE_AUTH_CONNECT_V6;
  const GUID layerKeyIn = isV4 ? FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4
                               : FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.action.type = FWP_ACTION_BLOCK;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;

  FWPM_FILTER_CONDITION0 cond[1] = {};
  FWP_RANGE0 ipRange;
  QByteArray lowIpV6Buffer;
  QByteArray highIpV6Buffer;

  importAddress(lower, ipRange.valueLow, &lowIpV6Buffer);
  importAddress(upper, ipRange.valueHigh, &highIpV6Buffer);

  cond[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
  cond[0].matchType = FWP_MATCH_RANGE;
  cond[0].conditionValue.type = FWP_RANGE_TYPE;
  cond[0].conditionValue.rangeValue = &ipRange;

  filter.numFilterConditions = 1;
  filter.filterCondition = cond;

  filter.layerKey = layerKeyOut;
  if (!enableFilter(&filter, title, description.arg("to").arg(addr.toString()),
                    peer)) {
    return false;
  }
  filter.layerKey = layerKeyIn;
  if (!enableFilter(&filter, title,
                    description.arg("from").arg(addr.toString()), peer)) {
    return false;
  }
  return true;
}

bool WindowsFirewall::blockTrafficTo(const QList<IPAddress>& rangeList,
                                     uint8_t weight, const QString& title,
                                     const QString& peer) {
  for (auto range : rangeList) {
    if (!blockTrafficTo(range, weight, title, peer)) {
      logger.info() << "Setting Range of" << range.toString() << "failed";
      return false;
    }
  }
  return true;
}

// Returns the Path of the Current Executable this runs in
QString WindowsFirewall::getCurrentPath() {
  const unsigned char initValue = 0xff;
  QByteArray buffer(2048, initValue);
  auto ok = GetModuleFileNameA(NULL, buffer.data(), buffer.size());

  if (ok == ERROR_INSUFFICIENT_BUFFER) {
    buffer.resize(buffer.size() * 2);
    ok = GetModuleFileNameA(NULL, buffer.data(), buffer.size());
  }
  if (ok == 0) {
    WindowsUtils::windowsLog("Err fetching dos path");
    return "";
  }

  return QString::fromLocal8Bit(buffer);
}

void WindowsFirewall::importAddress(const QHostAddress& addr,
                                    OUT FWP_VALUE0_& value,
                                    OUT QByteArray* v6DataBuffer) {
  const bool isV4 = addr.protocol() == QAbstractSocket::IPv4Protocol;
  if (isV4) {
    value.type = FWP_UINT32;
    value.uint32 = addr.toIPv4Address();
    return;
  }
  auto v6bytes = addr.toIPv6Address();
  v6DataBuffer->append((const char*)v6bytes.c, IPV6_ADDRESS_SIZE);
  value.type = FWP_BYTE_ARRAY16_TYPE;
  value.byteArray16 = (FWP_BYTE_ARRAY16*)v6DataBuffer->data();
}
void WindowsFirewall::importAddress(const QHostAddress& addr,
                                    OUT FWP_CONDITION_VALUE0_& value,
                                    OUT QByteArray* v6DataBuffer) {
  const bool isV4 = addr.protocol() == QAbstractSocket::IPv4Protocol;
  if (isV4) {
    value.type = FWP_UINT32;
    value.uint32 = addr.toIPv4Address();
    return;
  }
  auto v6bytes = addr.toIPv6Address();
  v6DataBuffer->append((const char*)v6bytes.c, IPV6_ADDRESS_SIZE);
  value.type = FWP_BYTE_ARRAY16_TYPE;
  value.byteArray16 = (FWP_BYTE_ARRAY16*)v6DataBuffer->data();
}

bool WindowsFirewall::blockTrafficOnPort(uint port, uint8_t weight,
                                         const QString& title) {
  // Allow Traffic to IP with PORT using any protocol
  FWPM_FILTER_CONDITION0 conds[3];
  conds[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
  conds[0].matchType = FWP_MATCH_EQUAL;
  conds[0].conditionValue.type = FWP_UINT8;
  conds[0].conditionValue.uint8 = (IPPROTO_UDP);

  conds[1].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
  conds[1].matchType = FWP_MATCH_EQUAL;
  conds[1].conditionValue.type = FWP_UINT8;
  conds[1].conditionValue.uint8 = (IPPROTO_TCP);

  conds[2].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
  conds[2].matchType = FWP_MATCH_EQUAL;
  conds[2].conditionValue.type = FWP_UINT16;
  conds[2].conditionValue.uint16 = port;

  // Assemble the Filter base
  FWPM_FILTER0 filter;
  memset(&filter, 0, sizeof(filter));
  filter.filterCondition = conds;
  filter.numFilterConditions = 3;
  filter.action.type = FWP_ACTION_BLOCK;
  filter.weight.type = FWP_UINT8;
  filter.weight.uint8 = weight;
  filter.subLayerKey = ST_FW_WINFW_BASELINE_SUBLAYER_KEY;

  QString description("Block %1 on Port %2");
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
  if (!enableFilter(&filter, title, description.arg("outgoing v6").arg(port))) {
    return false;
  }
  filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
  if (!enableFilter(&filter, title, description.arg("outgoing v4").arg(port))) {
    return false;
  }

  filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
  if (!enableFilter(&filter, title, description.arg("incoming v4").arg(port))) {
    return false;
  }
  filter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
  if (!enableFilter(&filter, title, description.arg("incoming v6").arg(port))) {
    return false;
  }
  return true;
}

bool WindowsFirewall::enableFilter(FWPM_FILTER0* filter, const QString& title,
                                   const QString& description,
                                   const QString& peer) {
  uint64_t filterID = 0;
  auto name = title.toStdWString();
  auto desc = description.toStdWString();
  filter->displayData.name = (PWSTR)name.c_str();
  filter->displayData.description = (PWSTR)desc.c_str();
  auto result = FwpmFilterAdd0(m_sessionHandle, filter, NULL, &filterID);
  if (result != ERROR_SUCCESS) {
    logger.error() << "Failed to enable filter: " << title << " "
                   << description;
    return false;
  }
  logger.info() << "Filter added: " << title << ":" << description;
  if (peer.isEmpty()) {
    m_activeRules.append(filterID);
  } else {
    m_peerRules.insert(peer, filterID);
  }
  return true;
}

bool WindowsFirewall::allowLoopbackTraffic(uint8_t weight,
                                           const QString& title) {
  QList<QNetworkInterface> networkInterfaces =
      QNetworkInterface::allInterfaces();
  for (const auto& iface : networkInterfaces) {
    if (iface.type() != QNetworkInterface::Loopback) {
      continue;
    }
    if (!allowTrafficOfAdapter(iface.index(), weight,
                               title.arg(iface.name()))) {
      return false;
    }
  }
  return true;
}
