/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowssplittunnel.h"

#include <qassert.h>

#include <memory>

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

#pragma region

// Driver Configuration structures
using CONFIGURATION_ENTRY = struct {
  // Offset into buffer region that follows all entries.
  // The image name uses the device path.
  SIZE_T ImageNameOffset;
  // Length of the String
  USHORT ImageNameLength;
};

using CONFIGURATION_HEADER = struct {
  // Number of entries immediately following the header.
  SIZE_T NumEntries;

  // Total byte length: header + entries + string buffer.
  SIZE_T TotalLength;
};

// Used to Configure Which IP is network/vpn
using IP_ADDRESSES_CONFIG = struct {
  IN_ADDR TunnelIpv4;
  IN_ADDR InternetIpv4;

  IN6_ADDR TunnelIpv6;
  IN6_ADDR InternetIpv6;
};

// Used to Define Which Processes are alive on activation
using PROCESS_DISCOVERY_HEADER = struct {
  SIZE_T NumEntries;
  SIZE_T TotalLength;
};

using PROCESS_DISCOVERY_ENTRY = struct {
  HANDLE ProcessId;
  HANDLE ParentProcessId;

  SIZE_T ImageNameOffset;
  USHORT ImageNameLength;
};

using ProcessInfo = struct {
  DWORD ProcessId;
  DWORD ParentProcessId;
  FILETIME CreationTime;
  std::wstring DevicePath;
};

#ifndef CTL_CODE

#  define FILE_ANY_ACCESS 0x0000

#  define METHOD_BUFFERED 0
#  define METHOD_IN_DIRECT 1
#  define METHOD_NEITHER 3

#  define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// Known ControlCodes
#define IOCTL_INITIALIZE CTL_CODE(0x8000, 1, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_DEQUEUE_EVENT \
  CTL_CODE(0x8000, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_REGISTER_PROCESSES \
  CTL_CODE(0x8000, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_REGISTER_IP_ADDRESSES \
  CTL_CODE(0x8000, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_IP_ADDRESSES \
  CTL_CODE(0x8000, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_CONFIGURATION \
  CTL_CODE(0x8000, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_CONFIGURATION \
  CTL_CODE(0x8000, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CLEAR_CONFIGURATION \
  CTL_CODE(0x8000, 8, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_GET_STATE CTL_CODE(0x8000, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_QUERY_PROCESS \
  CTL_CODE(0x8000, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ST_RESET CTL_CODE(0x8000, 11, METHOD_NEITHER, FILE_ANY_ACCESS)

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
  auto ok = GetProcessTimes(process, &creationTime, &null_time, &null_time,
                            &null_time);
  if (ok) {
    pi.CreationTime = creationTime;
  }
  wchar_t imagepath[MAX_PATH + 1];
  if (K32GetProcessImageFileNameW(
          process, imagepath, sizeof(imagepath) / sizeof(*imagepath)) != 0) {
    pi.DevicePath = imagepath;
  }
  return pi;
}

}  // namespace

std::unique_ptr<WindowsSplitTunnel> WindowsSplitTunnel::create(
    WindowsFirewall* fw) {
  if (fw == nullptr) {
    // Pre-Condition:
    // Make sure the Windows Firewall has created the sublayer
    // otherwise the driver will fail to initialize
    logger.error() << "Failed to did not pass a WindowsFirewall obj"
                   << "The Driver cannot work with the sublayer not created";
    return nullptr;
  }
  // 00: Check if we conflict with mullvad, if so.
  if (detectConflict()) {
    logger.error() << "Conflict detected, abort Split-Tunnel init.";
    return nullptr;
  }
  // 01: Check if the driver is installed, if not do so.
  if (!isInstalled()) {
    logger.debug() << "Driver is not Installed, doing so";
    auto handle = installDriver();
    if (handle == INVALID_HANDLE_VALUE) {
      WindowsUtils::windowsLog("Failed to install Driver");
      return nullptr;
    }
    logger.debug() << "Driver installed";
    CloseServiceHandle(handle);
  } else {
    logger.debug() << "Driver was installed";
  }
  // 02: Now check if the service is running
  auto driver_manager =
      WindowsServiceManager::open(QString::fromWCharArray(DRIVER_SERVICE_NAME));
  if (Q_UNLIKELY(driver_manager == nullptr)) {
    // Let's be fair if we end up here,
    // after checking it exists and installing it,
    // this is super unlikeley
    Q_ASSERT(false);
    logger.error()
        << "WindowsServiceManager was unable fo find Split Tunnel service?";
    return nullptr;
  }
  if (!driver_manager->isRunning()) {
    logger.debug() << "Driver is not running, starting it";
    // Start the service
    if (!driver_manager->startService()) {
      logger.error() << "Failed to start Split Tunnel Service";
      return nullptr;
    };
  }
  // 03: Open the Driver Symlink
  auto driverFile = CreateFileW(DRIVER_SYMLINK, GENERIC_READ | GENERIC_WRITE, 0,
                                nullptr, OPEN_EXISTING, 0, nullptr);
  ;
  if (driverFile == INVALID_HANDLE_VALUE) {
    WindowsUtils::windowsLog("Failed to open Driver: ");
    // Only once, if the opening did not work. Try to reboot it. #
    logger.info()
        << "Failed to open driver, attempting only once to reboot driver";
    if (!driver_manager->stopService()) {
      logger.error() << "Unable stop driver";
      return nullptr;
    };
    logger.info() << "Stopped driver, starting it again.";
    if (!driver_manager->startService()) {
      logger.error() << "Unable start driver";
      return nullptr;
    };
    logger.info() << "Opening again.";
    driverFile = CreateFileW(DRIVER_SYMLINK, GENERIC_READ | GENERIC_WRITE, 0,
                             nullptr, OPEN_EXISTING, 0, nullptr);
    if (driverFile == INVALID_HANDLE_VALUE) {
      logger.error() << "Opening Failed again, sorry!";
      return nullptr;
    }
  }
  if (!initDriver(driverFile)) {
    logger.error() << "Failed to init driver";
    return nullptr;
  }
  // We're ready to talk to the driver, it's alive and setup.
  return std::make_unique<WindowsSplitTunnel>(driverFile);
}

bool WindowsSplitTunnel::initDriver(HANDLE driverIO) {
  // We need to now check the state and init it, if required
  auto state = getState(driverIO);
  if (state == STATE_UNKNOWN) {
    logger.debug() << "Cannot check if driver is initialized";
    return false;
  }
  if (state >= STATE_INITIALIZED) {
    logger.debug() << "Driver already initialized: " << state;
    // Reset Driver as it has wfp handles probably >:(
    resetDriver(driverIO);

    auto newState = getState(driverIO);
    logger.debug() << "New state after reset:" << newState;
    if (newState >= STATE_INITIALIZED) {
      logger.debug() << "Reset unsuccesfull";
      return false;
    }
  }

  DWORD bytesReturned;
  auto ok = DeviceIoControl(driverIO, IOCTL_INITIALIZE, nullptr, 0, nullptr, 0,
                            &bytesReturned, nullptr);
  if (!ok) {
    auto err = GetLastError();
    logger.error() << "Driver init failed err -" << err;
    logger.error() << "State:" << getState(driverIO);

    return false;
  }
  logger.debug() << "Driver initialized" << getState(driverIO);
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

bool WindowsSplitTunnel::excludeApps(const QStringList& appPaths) {
  auto state = getState();
  if (state != STATE_READY && state != STATE_RUNNING) {
    logger.warning() << "Driver is not in the right State to set Rules"
                     << state;
    return false;
  }

  logger.debug() << "Pushing new Ruleset for Split-Tunnel " << state;
  auto config = generateAppConfiguration(appPaths);

  DWORD bytesReturned;
  auto ok = DeviceIoControl(m_driver, IOCTL_SET_CONFIGURATION, &config[0],
                            (DWORD)config.size(), nullptr, 0, &bytesReturned,
                            nullptr);
  if (!ok) {
    auto err = GetLastError();
    WindowsUtils::windowsLog("Set Config Failed:");
    logger.error() << "Failed to set Config err code " << err;
    return false;
  }
  logger.debug() << "New Configuration applied: " << stateString();
  return true;
}

bool WindowsSplitTunnel::start(int inetAdapterIndex, int vpnAdapterIndex) {
  // To Start we need to send 2 things:
  // Network info (what is vpn what is network)
  logger.debug() << "Starting SplitTunnel";
  DWORD bytesReturned;

  if (getState() == STATE_STARTED) {
    logger.debug() << "Driver needs Init Call";
    DWORD bytesReturned;
    auto ok = DeviceIoControl(m_driver, IOCTL_INITIALIZE, nullptr, 0, nullptr,
                              0, &bytesReturned, nullptr);
    if (!ok) {
      logger.error() << "Driver init failed";
      return false;
    }
  }

  // Process Info (what is running already)
  if (getState() == STATE_INITIALIZED) {
    logger.debug() << "State is Init, requires process config";
    auto config = generateProcessBlob();
    auto ok = DeviceIoControl(m_driver, IOCTL_REGISTER_PROCESSES, &config[0],
                              (DWORD)config.size(), nullptr, 0, &bytesReturned,
                              nullptr);
    if (!ok) {
      logger.error() << "Failed to set Process Config";
      return false;
    }
    logger.debug() << "Set Process Config ok || new State:" << stateString();
  }

  if (getState() == STATE_INITIALIZED) {
    logger.warning() << "Driver is still not ready after process list send";
    return false;
  }
  logger.debug() << "Driver is  ready || new State:" << stateString();

  auto config = generateIPConfiguration(inetAdapterIndex, vpnAdapterIndex);
  auto ok = DeviceIoControl(m_driver, IOCTL_REGISTER_IP_ADDRESSES, &config[0],
                            (DWORD)config.size(), nullptr, 0, &bytesReturned,
                            nullptr);
  if (!ok) {
    logger.error() << "Failed to set Network Config";
    return false;
  }
  logger.debug() << "New Network Config Applied || new State:" << stateString();
  return true;
}

void WindowsSplitTunnel::stop() {
  DWORD bytesReturned;
  auto ok = DeviceIoControl(m_driver, IOCTL_CLEAR_CONFIGURATION, nullptr, 0,
                            nullptr, 0, &bytesReturned, nullptr);
  if (!ok) {
    logger.error() << "Stopping Split tunnel not successfull";
    return;
  }
  logger.debug() << "Stopping Split tunnel successfull";
}

bool WindowsSplitTunnel::resetDriver(HANDLE driverIO) {
  DWORD bytesReturned;
  auto ok = DeviceIoControl(driverIO, IOCTL_ST_RESET, nullptr, 0, nullptr, 0,
                            &bytesReturned, nullptr);
  if (!ok) {
    logger.error() << "Reset Split tunnel not successfull";
    return false;
  }
  logger.debug() << "Reset Split tunnel successfull";
  return true;
}

// static
WindowsSplitTunnel::DRIVER_STATE WindowsSplitTunnel::getState(HANDLE driverIO) {
  if (driverIO == INVALID_HANDLE_VALUE) {
    logger.debug() << "Can't query State from non Opened Driver";
    return STATE_UNKNOWN;
  }
  DWORD bytesReturned;
  SIZE_T outBuffer;
  bool ok = DeviceIoControl(driverIO, IOCTL_GET_STATE, nullptr, 0, &outBuffer,
                            sizeof(outBuffer), &bytesReturned, nullptr);
  if (!ok) {
    WindowsUtils::windowsLog("getState response failure");
    return STATE_UNKNOWN;
  }
  if (bytesReturned == 0) {
    WindowsUtils::windowsLog("getState response is empty");
    return STATE_UNKNOWN;
  }
  return static_cast<WindowsSplitTunnel::DRIVER_STATE>(outBuffer);
}
WindowsSplitTunnel::DRIVER_STATE WindowsSplitTunnel::getState() {
  return getState(m_driver);
}

std::vector<uint8_t> WindowsSplitTunnel::generateAppConfiguration(
    const QStringList& appPaths) {
  // Step 1: Calculate how much size the buffer will need
  size_t cummulated_string_size = 0;
  QStringList dosPaths;
  for (auto const& path : appPaths) {
    auto dosPath = convertPath(path);
    dosPaths.append(dosPath);
    cummulated_string_size += dosPath.toStdWString().size() * sizeof(wchar_t);
    logger.debug() << dosPath;
  }
  size_t bufferSize = sizeof(CONFIGURATION_HEADER) +
                      (sizeof(CONFIGURATION_ENTRY) * appPaths.size()) +
                      cummulated_string_size;
  std::vector<uint8_t> outBuffer(bufferSize);

  auto header = (CONFIGURATION_HEADER*)&outBuffer[0];
  auto entry = (CONFIGURATION_ENTRY*)(header + 1);

  auto stringDest = &outBuffer[0] + sizeof(CONFIGURATION_HEADER) +
                    (sizeof(CONFIGURATION_ENTRY) * appPaths.size());

  SIZE_T stringOffset = 0;

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

  return outBuffer;
}

std::vector<std::byte> WindowsSplitTunnel::generateIPConfiguration(
    int inetAdapterIndex, int vpnAdapterIndex) {
  std::vector<std::byte> out(sizeof(IP_ADDRESSES_CONFIG));

  auto config = reinterpret_cast<IP_ADDRESSES_CONFIG*>(&out[0]);

  auto ifaces = QNetworkInterface::allInterfaces();

    if (vpnAdapterIndex == 0) {
    vpnAdapterIndex = WindowsCommons::VPNAdapterIndex();
  }
  // Always the VPN
  if (!getAddress(vpnAdapterIndex, &config->TunnelIpv4,
                  &config->TunnelIpv6)) {
    return {};
  }
  // 2nd best route is usually the internet adapter
  if (!getAddress(inetAdapterIndex, &config->InternetIpv4,
                  &config->InternetIpv6)) {
    return {};
  };
  return out;
}
bool WindowsSplitTunnel::getAddress(int adapterIndex, IN_ADDR* out_ipv4,
                                    IN6_ADDR* out_ipv6) {
  QNetworkInterface target =
      QNetworkInterface::interfaceFromIndex(adapterIndex);
  logger.debug() << "Getting adapter info for:" << target.humanReadableName();

  auto get = [&target](QAbstractSocket::NetworkLayerProtocol protocol) {
    for (auto address : target.addressEntries()) {
      if (address.ip().protocol() != protocol) {
        continue;
      }
      return address.ip().toString().toStdWString();
    }
    return std::wstring{};
  };
  auto ipv4 = get(QAbstractSocket::IPv4Protocol);
  auto ipv6 = get(QAbstractSocket::IPv6Protocol);

  if (InetPtonW(AF_INET, ipv4.c_str(), out_ipv4) != 1) {
    logger.debug() << "Ipv4 Conversation error" << WSAGetLastError();
    return false;
  }
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
  // Get a Snapshot of all processes that are running:
  HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot_handle == INVALID_HANDLE_VALUE) {
    WindowsUtils::windowsLog("Creating Process snapshot failed");
    return std::vector<uint8_t>(0);
  }
  auto cleanup = qScopeGuard([&] { CloseHandle(snapshot_handle); });
  // Load the First Entry, later iterate over all
  PROCESSENTRY32W currentProcess;
  currentProcess.dwSize = sizeof(PROCESSENTRY32W);

  if (FALSE == (Process32First(snapshot_handle, &currentProcess))) {
    WindowsUtils::windowsLog("Cant read first entry");
  }

  QMap<DWORD, ProcessInfo> processes;

  do {
    auto process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                      currentProcess.th32ProcessID);

    if (process_handle == INVALID_HANDLE_VALUE) {
      continue;
    }
    ProcessInfo info = getProcessInfo(process_handle, currentProcess);
    processes.insert(info.ProcessId, info);
    CloseHandle(process_handle);

  } while (FALSE != (Process32NextW(snapshot_handle, &currentProcess)));

  auto process_list = processes.values();
  if (process_list.isEmpty()) {
    logger.debug() << "Process Snapshot list was empty";
    return std::vector<uint8_t>(0);
  }

  logger.debug() << "Reading Processes NUM: " << process_list.size();
  // Determine the Size of the outBuffer:
  size_t totalStringSize = 0;

  for (const auto& process : process_list) {
    totalStringSize += (process.DevicePath.size() * sizeof(wchar_t));
  }
  auto bufferSize = sizeof(PROCESS_DISCOVERY_HEADER) +
                    (sizeof(PROCESS_DISCOVERY_ENTRY) * processes.size()) +
                    totalStringSize;

  std::vector<uint8_t> out(bufferSize);

  auto header = reinterpret_cast<PROCESS_DISCOVERY_HEADER*>(&out[0]);
  auto entry = reinterpret_cast<PROCESS_DISCOVERY_ENTRY*>(header + 1);
  auto stringBuffer = reinterpret_cast<uint8_t*>(entry + processes.size());

  SIZE_T currentStringOffset = 0;

  for (const auto& process : process_list) {
    // Wierd DWORD -> Handle Pointer magic.
    entry->ProcessId = (HANDLE)((size_t)process.ProcessId);
    entry->ParentProcessId = (HANDLE)((size_t)process.ParentProcessId);

    if (process.DevicePath.empty()) {
      entry->ImageNameOffset = 0;
      entry->ImageNameLength = 0;
    } else {
      const auto imageNameLength = process.DevicePath.size() * sizeof(wchar_t);

      entry->ImageNameOffset = currentStringOffset;
      entry->ImageNameLength = static_cast<USHORT>(imageNameLength);

      RtlCopyMemory(stringBuffer + currentStringOffset, &process.DevicePath[0],
                    imageNameLength);

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
  if (!driver.exists()) {
    logger.error() << "Split Tunnel Driver File not found "
                   << driver.absoluteFilePath();
    return (SC_HANDLE)INVALID_HANDLE_VALUE;
  }
  auto path = driver.absolutePath() + "/" + DRIVER_FILENAME;
  auto binPath = (const wchar_t*)path.utf16();
  auto scm_rights = SC_MANAGER_ALL_ACCESS;
  auto serviceManager = OpenSCManager(nullptr,  // local computer
                                      nullptr,  // servicesActive database
                                      scm_rights);
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
  auto serviceManager = OpenSCManager(NULL,  // local computer
                                      NULL,  // servicesActive database
                                      scm_rights);

  auto servicehandle =
      OpenService(serviceManager, DRIVER_SERVICE_NAME, GENERIC_READ);
  auto result = DeleteService(servicehandle);
  if (result) {
    logger.debug() << "Split Tunnel Driver Removed";
  }
  return result;
}
// static
bool WindowsSplitTunnel::isInstalled() {
  // Check if the Drivers I/O File is present
  auto symlink = QFileInfo(QString::fromWCharArray(DRIVER_SYMLINK));
  if (symlink.exists()) {
    return true;
  }
  // If not check with SCM, if the kernel service exists
  auto scm_rights = SC_MANAGER_ALL_ACCESS;
  auto serviceManager = OpenSCManager(NULL,  // local computer
                                      NULL,  // servicesActive database
                                      scm_rights);
  auto servicehandle =
      OpenService(serviceManager, DRIVER_SERVICE_NAME, GENERIC_READ);
  auto err = GetLastError();
  CloseServiceHandle(serviceManager);
  CloseServiceHandle(servicehandle);
  return err != ERROR_SERVICE_DOES_NOT_EXIST;
}

QString WindowsSplitTunnel::convertPath(const QString& path) {
  auto parts = path.split("/");
  QString driveLetter = parts.takeFirst();
  if (!driveLetter.contains(":") || parts.size() == 0) {
    // device should contain : for e.g C:
    return "";
  }
  QByteArray buffer(2048, 0xFFu);
  auto ok = QueryDosDeviceW(qUtf16Printable(driveLetter),
                            (wchar_t*)buffer.data(), buffer.size() / 2);

  if (ok == ERROR_INSUFFICIENT_BUFFER) {
    buffer.resize(buffer.size() * 2);
    ok = QueryDosDeviceW(qUtf16Printable(driveLetter), (wchar_t*)buffer.data(),
                         buffer.size() / 2);
  }
  if (ok == 0) {
    WindowsUtils::windowsLog("Err fetching dos path");
    return "";
  }
  QString deviceName;
  deviceName = QString::fromWCharArray((wchar_t*)buffer.data());
  parts.prepend(deviceName);

  return parts.join("\\");
}

// static
bool WindowsSplitTunnel::detectConflict() {
  auto scm_rights = SC_MANAGER_ENUMERATE_SERVICE;
  auto serviceManager = OpenSCManager(NULL,  // local computer
                                      NULL,  // servicesActive database
                                      scm_rights);
  auto cleanup = qScopeGuard([&] { CloseServiceHandle(serviceManager); });
  // Query for Mullvad Service.
  auto servicehandle =
      OpenService(serviceManager, MV_SERVICE_NAME, GENERIC_READ);
  auto err = GetLastError();
  CloseServiceHandle(servicehandle);
  if (err != ERROR_SERVICE_DOES_NOT_EXIST) {
    WindowsUtils::windowsLog("Mullvad Detected - Disabling SplitTunnel: ");
    // Mullvad is installed, so we would certainly break things.
    return true;
  }
  auto symlink = QFileInfo(QString::fromWCharArray(DRIVER_SYMLINK));
  if (!symlink.exists()) {
    // The driver is not loaded / installed.. MV is not installed, all good!
    logger.info() << "No Split-Tunnel Conflict detected, continue.";
    return false;
  }
  // The driver exists, so let's check if it has been created by us.
  // If our service is not present, it's has been created by
  // someone else so we should not use that :)
  servicehandle =
      OpenService(serviceManager, DRIVER_SERVICE_NAME, GENERIC_READ);
  err = GetLastError();
  CloseServiceHandle(servicehandle);
  return err == ERROR_SERVICE_DOES_NOT_EXIST;
}

bool WindowsSplitTunnel::isRunning() { return getState() == STATE_RUNNING; }
QString WindowsSplitTunnel::stateString() {
  switch (getState()) {
    case STATE_UNKNOWN:
      return "STATE_UNKNOWN";
    case STATE_NONE:
      return "STATE_NONE";
    case STATE_STARTED:
      return "STATE_STARTED";
    case STATE_INITIALIZED:
      return "STATE_INITIALIZED";
    case STATE_READY:
      return "STATE_READY";
    case STATE_RUNNING:
      return "STATE_RUNNING";
    case STATE_ZOMBIE:
      return "STATE_ZOMBIE";
      break;
  }
  return {};
}
