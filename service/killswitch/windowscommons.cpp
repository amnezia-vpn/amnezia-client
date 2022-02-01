/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowscommons.h"
//#include "logger.h"

#include <QDir>
#include <QtEndian>
#include <QHostAddress>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QNetworkInterface>

#include <Windows.h>
#include <iphlpapi.h>

#define TUNNEL_SERVICE_NAME L"WireGuardTunnel$mozvpn"

constexpr const char* VPN_NAME = "MozillaVPN";

constexpr const int WINDOWS_11_BUILD =
    22000;  // Build Number of the first release win 11 iso
//namespace {
//Logger logger(LOG_MAIN, "WindowsCommons");
//}

QString WindowsCommons::getErrorMessage() {
  DWORD errorId = GetLastError();
  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, errorId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&messageBuffer, 0, nullptr);

  std::string message(messageBuffer, size);
  QString result(message.c_str());
  LocalFree(messageBuffer);
  return result;
}

// A simple function to log windows error messages.
void WindowsCommons::windowsLog(const QString& msg) {
  QString errmsg = getErrorMessage();
  qDebug() << msg << "-" << errmsg;
}

QString WindowsCommons::tunnelConfigFile() {
  QStringList paths =
      QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
  for (const QString& path : paths) {
    QDir dir(path);
    if (!dir.exists()) {
      continue;
    }

    QDir vpnDir(dir.filePath(VPN_NAME));
    if (!vpnDir.exists()) {
      continue;
    }

    QString wireguardFile(vpnDir.filePath(QString("%1.conf").arg(VPN_NAME)));
    if (!QFileInfo::exists(wireguardFile)) {
      continue;
    }

    qDebug() << "Found the current wireguard configuration:"
                   << wireguardFile;
    return wireguardFile;
  }

  for (const QString& path : paths) {
    QDir dir(path);

    QDir vpnDir(dir.filePath(VPN_NAME));
    if (!vpnDir.exists() && !dir.mkdir(VPN_NAME)) {
     qDebug() << "Failed to create path Mozilla under" << path;
      continue;
    }

    return vpnDir.filePath(QString("%1.conf").arg(VPN_NAME));
  }

  qDebug() << "Failed to create the right paths";
  return QString();
}

QString WindowsCommons::tunnelLogFile() {
  QStringList paths =
      QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

  for (const QString& path : paths) {
    QDir dir(path);
    if (!dir.exists()) {
      continue;
    }

    QDir vpnDir(dir.filePath(VPN_NAME));
    if (!vpnDir.exists()) {
      continue;
    }

    return vpnDir.filePath("log.bin");
  }

  return QString();
}

// static
int WindowsCommons::AdapterIndexTo(const QHostAddress& dst) {
  qDebug() << "Getting Current Internet Adapter that routes to"
                 << dst.toString();
  quint32_be ipBigEndian;
  quint32 ip = dst.toIPv4Address();
  qToBigEndian(ip, &ipBigEndian);
  _MIB_IPFORWARDROW routeInfo;
  auto result = GetBestRoute(ipBigEndian, 0, &routeInfo);
  if (result != NO_ERROR) {
    return -1;
  }
  auto adapter =
      QNetworkInterface::interfaceFromIndex(routeInfo.dwForwardIfIndex);
  //logger.debug() << "Internet Adapter:" << adapter.name();
  qDebug()<< "Internet Adapter:" << adapter.name();
  return routeInfo.dwForwardIfIndex;
}

// static
int WindowsCommons::VPNAdapterIndex() {
  // For someReason QNetworkInterface::fromName(MozillaVPN) does not work >:(
  auto adapterList = QNetworkInterface::allInterfaces();
  for (const auto& adapter : adapterList) {
    if (
            adapter.humanReadableName().contains(IKEV2) ||
            adapter.humanReadableName().contains(IKEV2)
        ) {
      return adapter.index();
    }
  }
  return -1;
}

// Static
QString WindowsCommons::getCurrentPath() {
  QByteArray buffer(2048, 0xFF);
  auto ok = GetModuleFileNameA(NULL, buffer.data(), buffer.size());

  if (ok == ERROR_INSUFFICIENT_BUFFER) {
    buffer.resize(buffer.size() * 2);
    ok = GetModuleFileNameA(NULL, buffer.data(), buffer.size());
  }
  if (ok == 0) {
    WindowsCommons::windowsLog("Err fetching dos path");
    return "";
  }
  return QString::fromLocal8Bit(buffer);
}

// Static
QString WindowsCommons::WindowsVersion() {
  /* The Tradegy of Getting a somewhat working windows version:
    - GetVersion() -> deprecated and Reports win 8.1 for MozillaVPN... its tied
    to some .exe flags
    - NetWkstaGetInfo -> Reports Windows 10 on windows 11
    There is also the regirstry HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows
    NT\CurrentVersion
    -> CurrentMajorVersion reports 10 on win 11
    -> CurrentBuild seems to be correct, so lets infer it
  */

  QSettings regCurrentVersion(
      "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
      QSettings::NativeFormat);

  int buildNr = regCurrentVersion.value("CurrentBuild").toInt();
  if (buildNr >= WINDOWS_11_BUILD) {
    return "11";
  }
  return QSysInfo::productVersion();
}
