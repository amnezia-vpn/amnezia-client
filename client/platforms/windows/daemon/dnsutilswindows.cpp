/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilswindows.h"

#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2ipdef.h>

#include <QProcess>
#include <QTextStream>
#include <vector>

#include "leakdetector.h"
#include "logger.h"

constexpr uint32_t WINDOWS_NETSH_TIMEOUT_MSEC = 2000;

namespace {
Logger logger("DnsUtilsWindows");
}

DnsUtilsWindows::DnsUtilsWindows(QObject* parent) : DnsUtils(parent) {
  MZ_COUNT_CTOR(DnsUtilsWindows);
  logger.debug() << "DnsUtilsWindows created.";

  typedef DWORD WindowsSetDnsCallType(GUID, const void*);
  HMODULE library = LoadLibrary(TEXT("iphlpapi.dll"));
  if (library) {
    m_setInterfaceDnsSettingsProcAddr = (WindowsSetDnsCallType*)GetProcAddress(
        library, "SetInterfaceDnsSettings");
  }
}

DnsUtilsWindows::~DnsUtilsWindows() {
  MZ_COUNT_DTOR(DnsUtilsWindows);
  restoreResolvers();
  logger.debug() << "DnsUtilsWindows destroyed.";
}

QStringList DnsUtilsWindows::systemResolvers() const {
  ULONG size = 16 * 1024;
  std::vector<unsigned char> buffer(size);
  ULONG result = ERROR_BUFFER_OVERFLOW;
  for (int attempt = 0; attempt < 3 && result == ERROR_BUFFER_OVERFLOW; ++attempt) {
    buffer.resize(size);
    result = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
        nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
  }
  if (result != NO_ERROR) {
    logger.error() << "Cannot read system DNS:" << result;
    return {};
  }

  QStringList resolvers;
  const auto* adapters = reinterpret_cast<const IP_ADAPTER_ADDRESSES*>(buffer.data());
  for (const auto* adapter = adapters; adapter; adapter = adapter->Next) {
    // The loopback adapter can advertise obsolete fec0:: DNS placeholders.
    // Real loopback resolvers are listed on the connected network adapters.
    if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) {
      continue;
    }
    for (const auto* dns = adapter->FirstDnsServerAddress; dns; dns = dns->Next) {
      const auto* socketAddress = dns->Address.lpSockaddr;
      if (!socketAddress) {
        continue;
      }
      QHostAddress address;
      if (socketAddress->sa_family == AF_INET
          && dns->Address.iSockaddrLength >= sizeof(sockaddr_in)) {
        const auto* v4 = reinterpret_cast<const sockaddr_in*>(socketAddress);
        address.setAddress(ntohl(v4->sin_addr.s_addr));
      } else if (socketAddress->sa_family == AF_INET6
                 && dns->Address.iSockaddrLength >= sizeof(sockaddr_in6)) {
        const auto* v6 = reinterpret_cast<const sockaddr_in6*>(socketAddress);
        address.setAddress(reinterpret_cast<const quint8*>(&v6->sin6_addr));
        if (v6->sin6_scope_id) {
          address.setScopeId(QString::number(v6->sin6_scope_id));
        }
      }
      if (!address.isNull() && !address.isMulticast() && !address.isBroadcast()
          && address != QHostAddress(QHostAddress::AnyIPv4)
          && address != QHostAddress(QHostAddress::AnyIPv6)) {
        resolvers.append(address.toString());
      }
    }
  }
  resolvers.removeDuplicates();
  return resolvers;
}

bool DnsUtilsWindows::updateResolvers(const QString& ifname,
                                      const QList<QHostAddress>& resolvers) {
  MIB_IF_ROW2 entry;
  if (ConvertInterfaceAliasToLuid((wchar_t*)ifname.utf16(),
                                  &entry.InterfaceLuid) != 0) {
    logger.error() << "Failed to resolve LUID for" << ifname;
    return false;
  }
  if (GetIfEntry2(&entry) != NO_ERROR) {
    logger.error() << "Failed to resolve interface for" << ifname;
    return false;
  }
  m_luid = entry.InterfaceLuid.Value;

  logger.debug() << "Configuring DNS for" << ifname;
  if (m_setInterfaceDnsSettingsProcAddr == nullptr) {
    return updateResolversNetsh(entry.InterfaceIndex, resolvers);
  }
  return updateResolversWin32(entry.InterfaceGuid, resolvers);
}

bool DnsUtilsWindows::updateResolversWin32(
    GUID guid, const QList<QHostAddress>& resolvers) {
  QStringList v4resolvers;
  QStringList v6resolvers;
  for (const QHostAddress& addr : resolvers) {
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
      v4resolvers.append(addr.toString());
    }
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
      v6resolvers.append(addr.toString());
    }
  }

  DNS_INTERFACE_SETTINGS settings;
  settings.Version = DNS_INTERFACE_SETTINGS_VERSION1;
  settings.Flags = DNS_SETTING_NAMESERVER | DNS_SETTING_SEARCHLIST;
  settings.Domain = nullptr;
  settings.NameServer = nullptr;
  settings.SearchList = (wchar_t*)L".";
  settings.RegistrationEnabled = false;
  settings.RegisterAdapterName = false;
  settings.EnableLLMNR = false;
  settings.QueryAdapterName = false;
  settings.ProfileNameServer = nullptr;

  // Configure nameservers for IPv4
  QString v4resolverstring = v4resolvers.join(",");
  settings.NameServer = (wchar_t*)v4resolverstring.utf16();
  DWORD v4result = m_setInterfaceDnsSettingsProcAddr(guid, &settings);
  if (v4result != NO_ERROR) {
    logger.error() << "Failed to configure IPv4 resolvers:" << v4result;
  }

  // Configure nameservers for IPv6
  QString v6resolverstring = v6resolvers.join(",");
  settings.Flags |= DNS_SETTING_IPV6;
  settings.NameServer = (wchar_t*)v6resolverstring.utf16();
  DWORD v6result = m_setInterfaceDnsSettingsProcAddr(guid, &settings);
  if (v6result != NO_ERROR) {
    logger.error() << "Failed to configure IPv6 resolvers" << v6result;
  }

  return ((v4result == NO_ERROR) && (v6result == NO_ERROR));
}

constexpr const char* netshFlushTemplate =
    "interface %1 set dnsservers name=%2 address=none valdiate=no "
    "register=both\r\n";
constexpr const char* netshAddTemplate =
    "interface %1 add dnsservers name=%2 address=%3 validate=no\r\n";

bool DnsUtilsWindows::updateResolversNetsh(
    int ifindex, const QList<QHostAddress>& resolvers) {
  QProcess netsh;
  netsh.setProgram("netsh");
  netsh.start();
  if (!netsh.waitForStarted(WINDOWS_NETSH_TIMEOUT_MSEC)) {
    logger.error() << "Failed to start netsh";
    return false;
  }

  QTextStream cmdstream(&netsh);

  // Flush DNS servers
  QString v4flush = QString(netshFlushTemplate).arg("ipv4").arg(ifindex);
  QString v6flush = QString(netshFlushTemplate).arg("ipv6").arg(ifindex);
  logger.debug() << "netsh write:" << v4flush.trimmed();
  cmdstream << v4flush;
  logger.debug() << "netsh write:" << v6flush.trimmed();
  cmdstream << v6flush;

  // Add new DNS servers
  for (const QHostAddress& addr : resolvers) {
    const char* family = "ipv4";
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
      family = "ipv6";
    }
    QString nsAddr = addr.toString();
    QString nsCommand =
        QString(netshAddTemplate).arg(family).arg(ifindex).arg(nsAddr);
    logger.debug() << "netsh write:" << nsCommand.trimmed();
    cmdstream << nsCommand;
  }

  // Exit and cleanup netsh
  cmdstream << "exit\r\n";
  cmdstream.flush();
  if (!netsh.waitForFinished(WINDOWS_NETSH_TIMEOUT_MSEC)) {
    logger.error() << "Failed to exit netsh";
    return false;
  }

  return netsh.exitCode() == 0;
}

bool DnsUtilsWindows::restoreResolvers() {
  if (m_luid == 0) {
    // If the DNS hasn't been configured, there is nothing to restore.
    return true;
  }

  MIB_IF_ROW2 entry;
  DWORD error;
  entry.InterfaceLuid.Value = m_luid;
  error = GetIfEntry2(&entry);
  if (error == ERROR_FILE_NOT_FOUND) {
    // If the interface no longer exists, there is nothing to restore.
    return true;
  }
  if (error != NO_ERROR) {
    logger.error() << "Failed to resolve interface entry:" << error;
    return false;
  }

  QList<QHostAddress> empty;
  if (m_setInterfaceDnsSettingsProcAddr == nullptr) {
    return updateResolversNetsh(entry.InterfaceIndex, empty);
  }
  return updateResolversWin32(entry.InterfaceGuid, empty);
}
