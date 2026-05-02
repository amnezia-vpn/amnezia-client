/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowssplittunnel.h"

#include <qassert.h>
#include <memory>
#include <vector>

#include "../windowscommons.h"
#include "../windowsservicemanager.h"
#include "logger.h"
#include "platforms/windows/daemon/windowsfirewall.h"
#include "platforms/windows/daemon/windowssplittunnel.h"
#include "platforms/windows/windowsutils.h"
#include "windowsfirewall.h"

#define PSAPI_VERSION 2
#include <Windows.h>
#include <psapi.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QScopeGuard>
#include <combaseapi.h>

#include <fwpmu.h>
#pragma comment(lib, "Fwpuclnt.lib")

#pragma region

// Driver Configuration structures
using CONFIGURATION_ENTRY = struct {
  uint64_t ImageNameOffset;
  uint16_t ImageNameLength;
  uint8_t  _padding[6];
};

using CONFIGURATION_HEADER = struct {
  uint64_t NumEntries;
  uint64_t TotalLength;
  uint32_t SplitMode;
  uint32_t _padding; 
};

using IP_ADDRESSES_CONFIG = struct {
  IN_ADDR TunnelIpv4;
  IN_ADDR InternetIpv4;
  IN6_ADDR TunnelIpv6;
  IN6_ADDR InternetIpv6;
};

using PROCESS_DISCOVERY_HEADER = struct {
  uint64_t NumEntries;
  uint64_t TotalLength;
};

using PROCESS_DISCOVERY_ENTRY = struct {
  uint64_t ProcessId;
  uint64_t ParentProcessId;
  uint64_t ImageNameOffset;
  uint16_t ImageNameLength;
  uint8_t  _padding[6];
};

using ProcessInfo = struct {
  DWORD ProcessId;
  DWORD ParentProcessId;
  FILETIME CreationTime;
  std::wstring DevicePath;
};

using ST_SUBLAYER_GUIDS = struct {
  GUID Baseline;
  GUID Dns;
};

#ifndef CTL_CODE
#  define FILE_ANY_ACCESS 0x0000
#  define METHOD_BUFFERED 0
#  define METHOD_IN_DIRECT 1
#  define METHOD_NEITHER 3
#  define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// СТРОГО METHOD_BUFFERED! Я вернул макросы к рабочему 39 релизу
#define IOCTL_INITIALIZE CTL_CODE(0x8000, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEQUEUE_EVENT CTL_CODE(0x8000, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REGISTER_PROCESSES CTL_CODE(0x8000, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REGISTER_IP_ADDRESSES CTL_CODE(0x8000, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_IP_ADDRESSES CTL_CODE(0x8000, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_CONFIGURATION CTL_CODE(0x8000, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CONFIGURATION CTL_CODE(0x8000, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLEAR_CONFIGURATION CTL_CODE(0x8000, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_STATE CTL_CODE(0x8000, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUERY_PROCESS CTL_CODE(0x8000, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ST_RESET CTL_CODE(0x8000, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)

constexpr static const auto DRIVER_SYMLINK = L"\\\\.\\MULLVADSPLITTUNNEL";
constexpr static const auto DRIVER_FILENAME = "mullvad-split-tunnel.sys";
constexpr static const auto DRIVER_SERVICE_NAME = L"AmneziaVPNSplitTunnel";
constexpr static const auto MV_SERVICE_NAME = L"MullvadVPN";

#pragma endregion

namespace {
Logger logger("WindowsSplitTunnel");

ProcessInfo getProcessInfo(HANDLE process, const PROCESSENTRY32W& processMeta) {
  ProcessInfo pi;
  pi.ParentProcessId = processMeta.th32ParentProcessID;
  pi.ProcessId = processMeta.th32ProcessID;
  pi.CreationTime = {0, 0};
  pi.DevicePath = L"";

  FILETIME creationTime, null_time;
  auto ok = GetProcessTimes(process, &creationTime, &null_time, &null_time, &null_time);
  if (ok) {
    pi.CreationTime = creationTime;
  }
  wchar_t imagepath[MAX_PATH + 1];
  if (K32GetProcessImageFileNameW(process, imagepath, sizeof(imagepath) / sizeof(*imagepath)) != 0) {
    pi.DevicePath = imagepath;
  }
  return pi;
}

// Создаем слои, которые хардкодом зашиты в вашем ioctl.cpp
void RegisterHardcodedDriverSublayers() {
  HANDLE engine = NULL;
  if (FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engine) == ERROR_SUCCESS) {
      FWPM_SUBLAYER0 subLayer1;
      memset(&subLayer1, 0, sizeof(subLayer1));
      subLayer1.subLayerKey = { 0x11111111, 0x2222, 0x3333, { 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB } };
      subLayer1.displayData.name = (PWSTR)L"Amnezia ST Custom Baseline";
      subLayer1.weight = 0xFFFF;
      FwpmSubLayerAdd0(engine, &subLayer1, NULL);

      FWPM_SUBLAYER0 subLayer2;
      memset(&subLayer2, 0, sizeof(subLayer2));
      subLayer2.subLayerKey = { 0xAAAAAAAA, 0xBBBB, 0xCCCC, { 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44 } };
      subLayer2.displayData.name = (PWSTR)L"Amnezia ST Custom DNS";
      subLayer2.weight = 0xFFFF;
      FwpmSubLayerAdd0(engine, &subLayer2, NULL);

      FwpmEngineClose0(engine);
  }
}

}  // namespace

std::unique_ptr<WindowsSplitTunnel> WindowsSplitTunnel::create(WindowsFirewall* fw) {
  if (fw == nullptr) return nullptr;
  if (detectConflict()) return nullptr;
  if (!isInstalled()) {
    auto handle = installDriver();
    if (handle == INVALID_HANDLE_VALUE) return nullptr;
    CloseServiceHandle(handle);
  }
  auto driver_manager = WindowsServiceManager::open(QString::fromWCharArray(DRIVER_SERVICE_NAME));
  if (Q_UNLIKELY(driver_manager == nullptr)) return nullptr;
  
  if (!driver_manager->isRunning()) {
    if (!driver_manager->startService()) return nullptr;
  }
  
  auto driverFile = CreateFileW(DRIVER_SYMLINK, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (driverFile == INVALID_HANDLE_VALUE) {
    driver_manager->stopService();
    driver_manager->startService();
    driverFile = CreateFileW(DRIVER_SYMLINK, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (driverFile == INVALID_HANDLE_VALUE) return nullptr;
  }
  if (!initDriver(driverFile)) return nullptr;
  return std::make_unique<WindowsSplitTunnel>(driverFile);
}

bool WindowsSplitTunnel::initDriver(HANDLE driverIO) {
  logger.debug() << "[INIT] Ensuring Custom Driver WFP sublayers exist...";
  
  RegisterHardcodedDriverSublayers();

  DWORD bytesReturned = 0;
  DeviceIoControl(driverIO, IOCTL_ST_RESET, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
  
  ST_SUBLAYER_GUIDS guids;
  memset(&guids, 0, sizeof(guids));

  bool initOk = DeviceIoControl(driverIO, IOCTL_INITIALIZE, &guids, sizeof(guids), nullptr, 0, &bytesReturned, nullptr);
  
  if (!initOk) {
    DWORD err = GetLastError();
    if (err == 2150760457) { // FWP_E_ALREADY_EXISTS
      logger.info() << "[INIT] SUCCESS: Driver already initialized.";
      return true;
    }
    logger.error() << "[INIT] FATAL: Driver init failed, err: " << err;
    return false;
  }
  
  logger.debug() << "[INIT] SUCCESS: Driver initialized cleanly.";
  return true; 
}

WindowsSplitTunnel::WindowsSplitTunnel(HANDLE driverIO) : m_driver(driverIO) {
  logger.debug() << "Connected to the Driver";
  Q_ASSERT(getState() == STATE_INITIALIZED);
}

WindowsSplitTunnel::~WindowsSplitTunnel() {
  CloseHandle(m_driver);
  uninstallDriver();
}

bool WindowsSplitTunnel::excludeApps(const QStringList& appPaths, uint32_t splitMode) {
  auto state = getState();
  if (state != STATE_READY && state != STATE_RUNNING && state != STATE_INITIALIZED) {
    logger.warning() << "[EXCLUDE] Driver is not in the right State to set Rules: " << stateString();
    return false;
  }

  logger.debug() << "[EXCLUDE] Pushing Ruleset. Mode: " << splitMode << " | Apps count: " << appPaths.size();
  auto appConfig = generateAppConfiguration(appPaths, splitMode);

  DWORD bytesReturned = 0;
  DeviceIoControl(m_driver, IOCTL_CLEAR_CONFIGURATION, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);

  bool setOk = DeviceIoControl(m_driver, IOCTL_SET_CONFIGURATION, &appConfig[0], (DWORD)appConfig.size(), nullptr, 0, &bytesReturned, nullptr);
  if (!setOk) {
    DWORD err = GetLastError();
    WindowsUtils::windowsLog("Set Config Failed:");
    logger.error() << "[EXCLUDE] FATAL: Failed to set Config in Kernel! Err code: " << err;
    return false;
  }
  
  logger.debug() << "[EXCLUDE] SUCCESS! New Configuration applied perfectly.";
  return true;
}

bool WindowsSplitTunnel::start(int inetAdapterIndex, int vpnAdapterIndex) {
  logger.debug() << "[START] Starting SplitTunnel Configuration...";
  DWORD bytesReturned = 0;

  if (getState() == STATE_STARTED) {
    logger.debug() << "[START] Driver needs Init Call...";
    
    RegisterHardcodedDriverSublayers();
    DeviceIoControl(m_driver, IOCTL_ST_RESET, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
    
    ST_SUBLAYER_GUIDS guids;
    memset(&guids, 0, sizeof(guids));

    bool initOk = DeviceIoControl(m_driver, IOCTL_INITIALIZE, &guids, sizeof(guids), nullptr, 0, &bytesReturned, nullptr);
    if (!initOk) {
      DWORD err = GetLastError();
      if (err == 2150760457) {
         logger.info() << "[START] Driver already initialized. Proceeding.";
      } else {
         logger.error() << "[START] FATAL: Driver init failed, err: " << err;
         return false;
      }
    }
  }

  if (getState() == STATE_INITIALIZED) {
    logger.debug() << "[START] State is Init, sending active processes to driver...";
    auto processConfig = generateProcessBlob();
    if (!processConfig.empty()) {
        bool procOk = DeviceIoControl(m_driver, IOCTL_REGISTER_PROCESSES, &processConfig[0], (DWORD)processConfig.size(), nullptr, 0, &bytesReturned, nullptr);
        if(!procOk) logger.warning() << "[START] IOCTL_REGISTER_PROCESSES failed, err: " << GetLastError();
    }
  }

  logger.debug() << "[START] Waiting for VPN IP configuration (Loop)...";
  std::vector<std::byte> ipConfig;
  for (int i = 0; i < 15; ++i) { 
    ipConfig = generateIPConfiguration(inetAdapterIndex, vpnAdapterIndex);
    if (!ipConfig.empty()) break; 
    Sleep(500); 
  }

  if (ipConfig.empty()) {
      logger.error() << "[START] FATAL: Failed to get Network Config IPs after 15 tries.";
      return false;
  }

  logger.debug() << "[START] Sending IP Config to driver...";
  bool netOk = DeviceIoControl(m_driver, IOCTL_REGISTER_IP_ADDRESSES, &ipConfig[0], (DWORD)ipConfig.size(), nullptr, 0, &bytesReturned, nullptr);
  if (!netOk) {
      logger.error() << "[START] FATAL: Failed to set Network Config, err: " << GetLastError();
      return false;
  }
  
  logger.debug() << "[START] SUCCESS! Driver is ready || new State: " << stateString();
  return true;
}

void WindowsSplitTunnel::stop() {
  DWORD bytesReturned = 0;
  bool ok = DeviceIoControl(m_driver, IOCTL_CLEAR_CONFIGURATION, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
  if (!ok) {
    logger.error() << "Stopping Split tunnel not successfull";
    return;
  }
  logger.debug() << "Stopping Split tunnel successfull";
}

bool WindowsSplitTunnel::resetDriver(HANDLE driverIO) {
  DWORD bytesReturned = 0;
  bool ok = DeviceIoControl(driverIO, IOCTL_ST_RESET, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
  if (!ok) {
    logger.error() << "Reset Split tunnel not successfull";
    return false;
  }
  logger.debug() << "Reset Split tunnel successfull";
  return true;
}

WindowsSplitTunnel::DRIVER_STATE WindowsSplitTunnel::getState(HANDLE driverIO) {
  if (driverIO == INVALID_HANDLE_VALUE) return STATE_UNKNOWN;
  DWORD bytesReturned = 0;
  uint64_t outBuffer = 0; 
  
  bool ok = DeviceIoControl(driverIO, IOCTL_GET_STATE, nullptr, 0, &outBuffer, 8, &bytesReturned, nullptr);
  if (!ok || bytesReturned == 0) return STATE_UNKNOWN;
  return static_cast<WindowsSplitTunnel::DRIVER_STATE>(outBuffer);
}

WindowsSplitTunnel::DRIVER_STATE WindowsSplitTunnel::getState() {
  return getState(m_driver);
}

std::vector<uint8_t> WindowsSplitTunnel::generateAppConfiguration(const QStringList& appPaths, uint32_t splitMode) {
  size_t cummulated_string_size = 0;
  QStringList dosPaths;
  for (auto const& path : appPaths) {
    auto dosPath = convertPath(path);
    logger.debug() << "[EXCLUDE] Config Path Added: " << dosPath;
    dosPaths.append(dosPath);
    cummulated_string_size += dosPath.toStdWString().size() * sizeof(wchar_t);
  }
  size_t bufferSize = sizeof(CONFIGURATION_HEADER) + (sizeof(CONFIGURATION_ENTRY) * appPaths.size()) + cummulated_string_size;
  std::vector<uint8_t> outBuffer(bufferSize);

  auto header = (CONFIGURATION_HEADER*)&outBuffer[0];
  auto entry = (CONFIGURATION_ENTRY*)(header + 1);
  auto stringDest = &outBuffer[0] + sizeof(CONFIGURATION_HEADER) + (sizeof(CONFIGURATION_ENTRY) * appPaths.size());

  uint64_t stringOffset = 0;

  for (const QString& path : dosPaths) {
    auto wstr = path.toStdWString();
    auto cstr = wstr.c_str();
    auto stringLength = wstr.size() * sizeof(wchar_t);

    entry->ImageNameLength = (USHORT)stringLength;
    entry->ImageNameOffset = stringOffset;
    memcpy(stringDest, cstr, stringLength);

    ++entry;
    stringDest += stringLength;
    stringOffset += stringLength;
  }

  header->NumEntries = appPaths.length();
  header->TotalLength = bufferSize;
  header->SplitMode = splitMode;

  return outBuffer;
}

std::vector<std::byte> WindowsSplitTunnel::generateIPConfiguration(int inetAdapterIndex, int vpnAdapterIndex) {
  std::vector<std::byte> out(sizeof(IP_ADDRESSES_CONFIG));
  auto config = reinterpret_cast<IP_ADDRESSES_CONFIG*>(&out[0]);
  auto ifaces = QNetworkInterface::allInterfaces();

  if (vpnAdapterIndex == 0) vpnAdapterIndex = WindowsCommons::VPNAdapterIndex();
  
  if (!getAddress(vpnAdapterIndex, &config->TunnelIpv4, &config->TunnelIpv6)) return {};
  if (!getAddress(inetAdapterIndex, &config->InternetIpv4, &config->InternetIpv6)) return {};
  return out;
}

bool WindowsSplitTunnel::getAddress(int adapterIndex, IN_ADDR* out_ipv4, IN6_ADDR* out_ipv6) {
  QNetworkInterface target = QNetworkInterface::interfaceFromIndex(adapterIndex);

  auto get = [&target](QAbstractSocket::NetworkLayerProtocol protocol) {
    for (auto address : target.addressEntries()) {
      if (address.ip().protocol() != protocol) continue;
      return address.ip().toString().toStdWString();
    }
    return std::wstring{};
  };
  
  auto ipv4 = get(QAbstractSocket::IPv4Protocol);
  auto ipv6 = get(QAbstractSocket::IPv6Protocol);

  if (InetPtonW(AF_INET, ipv4.c_str(), out_ipv4) != 1) return false;
  
  if (ipv6.empty()) {
    std::memset(out_ipv6, 0x00, sizeof(IN6_ADDR));
    return true;
  }
  if (InetPtonW(AF_INET6, ipv6.c_str(), out_ipv6) != 1) {
    logger.debug() << "Ipv6 Conversation error" << WSAGetLastError();
  }
  return true;
}

std::vector<uint8_t> WindowsSplitTunnel::generateProcessBlob() {
  HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot_handle == INVALID_HANDLE_VALUE) return std::vector<uint8_t>(0);
  
  auto cleanup = qScopeGuard([&] { CloseHandle(snapshot_handle); });
  PROCESSENTRY32W currentProcess;
  currentProcess.dwSize = sizeof(PROCESSENTRY32W);

  if (FALSE == (Process32First(snapshot_handle, &currentProcess))) {
    WindowsUtils::windowsLog("Cant read first entry");
  }

  QMap<DWORD, ProcessInfo> processes;

  do {
    auto process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, currentProcess.th32ProcessID);
    if (process_handle == INVALID_HANDLE_VALUE) continue;
    
    ProcessInfo info = getProcessInfo(process_handle, currentProcess);
    processes.insert(info.ProcessId, info);
    CloseHandle(process_handle);

  } while (FALSE != (Process32NextW(snapshot_handle, &currentProcess)));

  auto process_list = processes.values();
  if (process_list.isEmpty()) return std::vector<uint8_t>(0);

  size_t totalStringSize = 0;
  for (const auto& process : process_list) {
    totalStringSize += (process.DevicePath.size() * sizeof(wchar_t));
  }
  auto bufferSize = sizeof(PROCESS_DISCOVERY_HEADER) + (sizeof(PROCESS_DISCOVERY_ENTRY) * processes.size()) + totalStringSize;

  std::vector<uint8_t> out(bufferSize);
  auto header = reinterpret_cast<PROCESS_DISCOVERY_HEADER*>(&out[0]);
  auto entry = reinterpret_cast<PROCESS_DISCOVERY_ENTRY*>(header + 1);
  auto stringBuffer = reinterpret_cast<uint8_t*>(entry + processes.size());

  uint64_t currentStringOffset = 0;

  for (const auto& process : process_list) {
    entry->ProcessId = (uint64_t)process.ProcessId;
    entry->ParentProcessId = (uint64_t)process.ParentProcessId;

    if (process.DevicePath.empty()) {
      entry->ImageNameOffset = 0;
      entry->ImageNameLength = 0;
    } else {
      const auto imageNameLength = process.DevicePath.size() * sizeof(wchar_t);
      entry->ImageNameOffset = currentStringOffset;
      entry->ImageNameLength = static_cast<USHORT>(imageNameLength);
      RtlCopyMemory(stringBuffer + currentStringOffset, &process.DevicePath[0], imageNameLength);
      currentStringOffset += imageNameLength;
    }
    ++entry;
  }

  header->NumEntries = processes.size();
  header->TotalLength = bufferSize;
  return out;
}

// static
SC_HANDLE WindowsSplitTunnel::installDriver() {
  LPCWSTR displayName = L"Amnezia Split Tunnel Service";
  QFileInfo driver(qApp->applicationDirPath() + "/" + DRIVER_FILENAME);
  if (!driver.exists()) return (SC_HANDLE)INVALID_HANDLE_VALUE;
  
  auto path = driver.absolutePath() + "/" + DRIVER_FILENAME;
  auto binPath = (const wchar_t*)path.utf16();
  auto scm_rights = SC_MANAGER_ALL_ACCESS;
  auto serviceManager = OpenSCManager(nullptr, nullptr, scm_rights);
  auto service = CreateService(
      serviceManager, DRIVER_SERVICE_NAME, displayName, SERVICE_ALL_ACCESS,
      SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, binPath,
      nullptr, nullptr, nullptr, nullptr, nullptr);
  CloseServiceHandle(serviceManager);
  return service;
}

// static
bool WindowsSplitTunnel::uninstallDriver() {
  auto scm_rights = SC_MANAGER_ALL_ACCESS;
  auto serviceManager = OpenSCManager(NULL, NULL, scm_rights);
  auto servicehandle = OpenService(serviceManager, DRIVER_SERVICE_NAME, GENERIC_READ);
  auto result = DeleteService(servicehandle);
  return result;
}

// static
bool WindowsSplitTunnel::isInstalled() {
  auto symlink = QFileInfo(QString::fromWCharArray(DRIVER_SYMLINK));
  if (symlink.exists()) return true;
  
  auto scm_rights = SC_MANAGER_ALL_ACCESS;
  auto serviceManager = OpenSCManager(NULL, NULL, scm_rights);
  auto servicehandle = OpenService(serviceManager, DRIVER_SERVICE_NAME, GENERIC_READ);
  auto err = GetLastError();
  CloseServiceHandle(serviceManager);
  CloseServiceHandle(servicehandle);
  return err != ERROR_SERVICE_DOES_NOT_EXIST;
}

QString WindowsSplitTunnel::convertPath(const QString& path) {
  QString nativePath = path;
  nativePath.replace("/", "\\");

  HANDLE hFile = CreateFileW(
      (const wchar_t*)nativePath.utf16(),
      0,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS,
      nullptr
  );

  // ИСПОЛЬЗУЕМ СИСТЕМНОЕ ИМЯ ЯДРА С ТОЧНЫМ РЕГИСТРОМ ДЛЯ ИДЕАЛЬНОГО СОВПАДЕНИЯ
  if (hFile != INVALID_HANDLE_VALUE) {
      std::vector<wchar_t> buffer(MAX_PATH);
      DWORD length = GetFinalPathNameByHandleW(hFile, buffer.data(), buffer.size(), VOLUME_NAME_NT);
      if (length > buffer.size()) {
          buffer.resize(length);
          length = GetFinalPathNameByHandleW(hFile, buffer.data(), buffer.size(), VOLUME_NAME_NT);
      }
      CloseHandle(hFile);

      if (length > 0) {
          QString result = QString::fromWCharArray(buffer.data(), length);
          return result.toLower();
      }
  }

  // Фолбэк, если файл занят
  auto parts = path.split("/");
  QString driveLetter = parts.takeFirst();
  if (!driveLetter.contains(":") || parts.size() == 0) return "";
  
  QByteArray buffer(2048, 0xFFu);
  auto ok = QueryDosDeviceW(qUtf16Printable(driveLetter), (wchar_t*)buffer.data(), buffer.size() / 2);

  if (ok == ERROR_INSUFFICIENT_BUFFER) {
    buffer.resize(buffer.size() * 2);
    ok = QueryDosDeviceW(qUtf16Printable(driveLetter), (wchar_t*)buffer.data(), buffer.size() / 2);
  }
  if (ok == 0) return "";
  
  QString deviceName = QString::fromWCharArray((wchar_t*)buffer.data());
  parts.prepend(deviceName);
  return parts.join("\\");
}

// static
bool WindowsSplitTunnel::detectConflict() {
  auto scm_rights = SC_MANAGER_ENUMERATE_SERVICE;
  auto serviceManager = OpenSCManager(NULL, NULL, scm_rights);
  auto cleanup = qScopeGuard([&] { CloseServiceHandle(serviceManager); });
  auto servicehandle = OpenService(serviceManager, MV_SERVICE_NAME, GENERIC_READ);
  auto err = GetLastError();
  CloseServiceHandle(servicehandle);
  if (err != ERROR_SERVICE_DOES_NOT_EXIST) return true;
  
  auto symlink = QFileInfo(QString::fromWCharArray(DRIVER_SYMLINK));
  if (!symlink.exists()) return false;
  
  servicehandle = OpenService(serviceManager, DRIVER_SERVICE_NAME, GENERIC_READ);
  err = GetLastError();
  CloseServiceHandle(servicehandle);
  return err == ERROR_SERVICE_DOES_NOT_EXIST;
}

bool WindowsSplitTunnel::isRunning() { return getState() == STATE_RUNNING; }

QString WindowsSplitTunnel::stateString() {
  switch (getState()) {
    case STATE_UNKNOWN: return "STATE_UNKNOWN";
    case STATE_NONE: return "STATE_NONE";
    case STATE_STARTED: return "STATE_STARTED";
    case STATE_INITIALIZED: return "STATE_INITIALIZED";
    case STATE_READY: return "STATE_READY";
    case STATE_RUNNING: return "STATE_RUNNING";
    case STATE_ZOMBIE: return "STATE_ZOMBIE";
  }
  return {};
}
