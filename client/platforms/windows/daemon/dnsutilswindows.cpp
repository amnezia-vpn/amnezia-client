/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dnsutilswindows.h"

#include <iphlpapi.h>
#include <windows.h>

#include <QProcess>
#include <QTextStream>

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

bool DnsUtilsWindows::updateResolvers(const QString& ifname,
                                      const QList<QHostAddress>& resolvers) {
  NET_LUID luid;
  if (ConvertInterfaceAliasToLuid((wchar_t*)ifname.utf16(), &luid) != 0) {
    logger.error() << "Failed to resolve LUID for" << ifname;
    return false;
  }
  m_luid = luid.Value;

  logger.debug() << "Configuring DNS for" << ifname;
  if (m_setInterfaceDnsSettingsProcAddr == nullptr) {
    return updateResolversNetsh(resolvers);
  }
  return updateResolversWin32(resolvers);
}

bool DnsUtilsWindows::updateResolversWin32(
    const QList<QHostAddress>& resolvers) {
  GUID guid;
  NET_LUID luid;
  luid.Value = m_luid;
  if (ConvertInterfaceLuidToGuid(&luid, &guid) != NO_ERROR) {
    logger.error() << "Failed to resolve GUID";
    return false;
  }

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
    const QList<QHostAddress>& resolvers) {
  QProcess netsh;
  NET_LUID luid;
  NET_IFINDEX ifindex;
  luid.Value = m_luid;
  if (ConvertInterfaceLuidToIndex(&luid, &ifindex) != NO_ERROR) {
    logger.error() << "Failed to resolve GUID";
    return false;
  }

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
    return true;
  }

  QList<QHostAddress> empty;
  if (m_setInterfaceDnsSettingsProcAddr == nullptr) {
    return updateResolversNetsh(empty);
  }
  return updateResolversWin32(empty);
}
