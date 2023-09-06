/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSSPLITTUNNEL_H
#define WINDOWSSPLITTUNNEL_H

#include <QObject>
#include <QString>
#include <QStringList>

// Note: the ws2tcpip.h import must come before the others.
// clang-format off
#include <ws2tcpip.h>
// clang-format on
#include <Ws2ipdef.h>
#include <ioapiset.h>
#include <tlhelp32.h>
#include <windows.h>

// States for GetState
enum DRIVER_STATE {
  STATE_UNKNOWN = -1,
  STATE_NONE = 0,
  STATE_STARTED = 1,
  STATE_INITIALIZED = 2,
  STATE_READY = 3,
  STATE_RUNNING = 4,
  STATE_ZOMBIE = 5,
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

// Driver Configuration structures

typedef struct {
  // Offset into buffer region that follows all entries.
  // The image name uses the device path.
  SIZE_T ImageNameOffset;
  // Length of the String
  USHORT ImageNameLength;
} CONFIGURATION_ENTRY;

typedef struct {
  // Number of entries immediately following the header.
  SIZE_T NumEntries;

  // Total byte length: header + entries + string buffer.
  SIZE_T TotalLength;
} CONFIGURATION_HEADER;

// Used to Configure Which IP is network/vpn
typedef struct {
  IN_ADDR TunnelIpv4;
  IN_ADDR InternetIpv4;

  IN6_ADDR TunnelIpv6;
  IN6_ADDR InternetIpv6;
} IP_ADDRESSES_CONFIG;

// Used to Define Which Processes are alive on activation
typedef struct {
  SIZE_T NumEntries;
  SIZE_T TotalLength;
} PROCESS_DISCOVERY_HEADER;

typedef struct {
  HANDLE ProcessId;
  HANDLE ParentProcessId;

  SIZE_T ImageNameOffset;
  USHORT ImageNameLength;
} PROCESS_DISCOVERY_ENTRY;

typedef struct {
  DWORD ProcessId;
  DWORD ParentProcessId;
  FILETIME CreationTime;
  std::wstring DevicePath;
} ProcessInfo;

class WindowsSplitTunnel final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(WindowsSplitTunnel)
 public:
  explicit WindowsSplitTunnel(QObject* parent);
  ~WindowsSplitTunnel();

  // void excludeApps(const QStringList& paths);
  // Excludes an Application from the VPN
  void setRules(const QStringList& appPaths);

  // Fetches and Pushed needed info to move to engaged mode
  void start(int inetAdapterIndex);
  // Deletes Rules and puts the driver into passive mode
  void stop();
  // Resets the Whole Driver
  void reset();

  // Just close connection, leave state as is
  void close();

  // Installes the Kernel Driver as Driver Service
  static SC_HANDLE installDriver();
  static bool uninstallDriver();
  static bool isInstalled();
  static bool detectConflict();

 private slots:
  void initDriver();

 private:
  HANDLE m_driver = INVALID_HANDLE_VALUE;
  constexpr static const auto DRIVER_SYMLINK = L"\\\\.\\MULLVADSPLITTUNNEL";
  constexpr static const auto DRIVER_FILENAME = "mullvad-split-tunnel.sys";
  constexpr static const auto DRIVER_SERVICE_NAME = L"MozillaVPNSplitTunnel";
  constexpr static const auto MV_SERVICE_NAME = L"MullvadVPN";
  DRIVER_STATE getState();

  // Initializes the WFP Sublayer
  bool initSublayer();

  // Generates a Configuration for Each APP
  std::vector<uint8_t> generateAppConfiguration(const QStringList& appPaths);
  // Generates a Configuration which IP's are VPN and which network
  std::vector<uint8_t> generateIPConfiguration(int inetAdapterIndex);
  std::vector<uint8_t> generateProcessBlob();

  void getAddress(int adapterIndex, IN_ADDR* out_ipv4, IN6_ADDR* out_ipv6);
  // Collects info about an Opened Process
  ProcessInfo getProcessInfo(HANDLE process,
                             const PROCESSENTRY32W& processMeta);

  // Converts a path to a Dos Path:
  // e.g C:/a.exe -> /harddisk0/a.exe
  QString convertPath(const QString& path);
};

#endif  // WINDOWSSPLITTUNNEL_H
