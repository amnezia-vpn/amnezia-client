/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowscommons.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <iphlpapi.h>
#include <shlobj_core.h>

#include <QDir>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QtEndian>

#include "logger.h"
#include "platforms/windows/windowsutils.h"

constexpr const char* VPN_NAME = "AmneziaVPN";
constexpr const char* WIREGUARD_DIR = "AmneziaWG";
constexpr const char* DATA_DIR = "Data";

namespace {
Logger logger("WindowsCommons");
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

    logger.debug() << "Found the current wireguard configuration:"
                   << wireguardFile;
    return wireguardFile;
  }

  for (const QString& path : paths) {
    QDir dir(path);

    QDir vpnDir(dir.filePath(VPN_NAME));
    if (!vpnDir.exists() && !dir.mkdir(VPN_NAME)) {
      logger.debug() << "Failed to create path Amnezia under" << path;
      continue;
    }

    return vpnDir.filePath(QString("%1.conf").arg(VPN_NAME));
  }

  logger.error() << "Failed to create the right paths";
  return QString();
}

// static
QString WindowsCommons::tunnelLogFile() {
  static QString tunnelLogFilePath = getTunnelLogFilePath();
  return tunnelLogFilePath;
}

// static
QString WindowsCommons::getProgramFilesPath() {
  wchar_t* path = nullptr;

  if (SUCCEEDED(
          SHGetKnownFolderPath(FOLDERID_ProgramFiles, 0, nullptr, &path))) {
    auto guard = qScopeGuard([&] { CoTaskMemFree(path); });
    return QString::fromWCharArray(path);
  }
  return QString();
}

// static
QString WindowsCommons::getTunnelLogFilePath() {
  // Return WireGuard's log file path, "\Program Files\WireGuard\Data\log.bin",
  // if the directory path exists
  auto programFilesPath = getProgramFilesPath();
  if (!programFilesPath.isEmpty()) {
    QDir programFilesDir(programFilesPath);

    if (programFilesDir.exists()) {
      QDir wireGuardDir(programFilesDir.filePath(WIREGUARD_DIR));

      if (wireGuardDir.exists()) {
        QDir wireGuardDataDir(wireGuardDir.filePath(DATA_DIR));

        if (wireGuardDataDir.exists()) {
          return wireGuardDataDir.filePath("log.bin");
        }
      }
    }
  }

  logger.debug() << "Failed to find WireGuard Tunnel log file";
  return QString();
}

// static
int WindowsCommons::AdapterIndexTo(const QHostAddress& dst) {
  logger.debug() << "Getting Current Internet Adapter that routes to"
                 << logger.sensitive(dst.toString());
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
  logger.debug() << "Internet Adapter:" << adapter.name();
  return routeInfo.dwForwardIfIndex;
}

// static
int WindowsCommons::VPNAdapterIndex() {
  // For someReason QNetworkInterface::fromName(MozillaVPN) does not work >:(
  auto adapterList = QNetworkInterface::allInterfaces();
  for (const auto& adapter : adapterList) {
    if (adapter.humanReadableName().contains("AmneziaVPN")) {
      return adapter.index();
    }
  }
  return -1;
}

// Static
QString WindowsCommons::getCurrentPath() {
  QByteArray buffer(2048, 0xFFu);
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

// Static
bool WindowsCommons::requireSoftwareRendering() {
  /* Qt6 appears to require Direct3D shader level 5, and can result in rendering
   * failures on some platforms. To workaround the issue, try to identify if
   * this device can reliably run the shaders, and request fallback to software
   * rendering if not.
   *
   * See: https://bugreports.qt.io/browse/QTBUG-100689
   */
  IDXGIFactory1* factory;
  HRESULT result;

  result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&factory));
  if (FAILED(result)) {
    logger.error() << "Failed to create DXGI Factory:" << result;
    return true;
  }
  auto guard = qScopeGuard([&] { factory->Release(); });

  // Enumerate the graphics adapters to find the minimum D3D shader version
  // that we can guarantee will render successfully.
  UINT i = 0;
  IDXGIAdapter1* adapter;
  D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_1;
  while (factory->EnumAdapters1(i++, &adapter) != DXGI_ERROR_NOT_FOUND) {
    auto adapterGuard = qScopeGuard([adapter] { adapter->Release(); });
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    QString gpuName = QString::fromWCharArray(desc.Description);

    // Try creating the driver to see what D3D feature level it supports.
    D3D_FEATURE_LEVEL gpuFeatureLevel = D3D_FEATURE_LEVEL_9_1;
    result = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                               nullptr, 0, D3D11_SDK_VERSION, nullptr,
                               &gpuFeatureLevel, nullptr);
    if (FAILED(result)) {
      logger.error() << "D3D Device" << gpuName
                     << "failed:" << QString::number((quint32)result, 16);
    } else {
      if (gpuFeatureLevel < minFeatureLevel) {
        minFeatureLevel = gpuFeatureLevel;
      }
      logger.debug() << "D3D Device" << gpuName
                     << "supports D3D:" << QString::number(gpuFeatureLevel, 16);
    }
  }

  // D3D version 11.0 shader level 5, is required for GPU rendering.
  return (minFeatureLevel < D3D_FEATURE_LEVEL_11_0);
}
