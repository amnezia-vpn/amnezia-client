/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSSPLITTUNNEL_H
#define WINDOWSSPLITTUNNEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <cstdint>
#include <memory>
#include <vector>

// Note: the ws2tcpip.h import must come before the others.
// clang-format off
#include <ws2tcpip.h>
// clang-format on
#include <Ws2ipdef.h>
#include <ioapiset.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <windows.h>

class WindowsFirewall;

class WindowsSplitTunnel final : public QObject {
 public:
  /**
   * @brief Installs and Initializes the Split Tunnel Driver.
   *
   * @param fw -
   * @return std::unique_ptr<WindowsSplitTunnel> - Is null on failure.
   */
  static std::unique_ptr<WindowsSplitTunnel> create(WindowsFirewall* fw);

  /**
   * @brief Construct a new Windows Split Tunnel object
   *
   * @param driverIO - The Handle to the Driver's IO file, it assumes the driver
   * is in STATE_INITIALIZED and the Firewall has been setup.
   * Prefer using create() to get to this state.
   */
  WindowsSplitTunnel(HANDLE driverIO);
  /**
   * @brief Destroy the Windows Split Tunnel object and uninstalls the Driver.
   */
  ~WindowsSplitTunnel();

  // void excludeApps(const QStringList& paths);
  // Excludes an Application from the VPN
  bool excludeApps(const QStringList& appPaths);

  // Fetches and Pushed needed info to move to engaged mode
  bool start(int inetAdapterIndex, int vpnAdapterIndex = 0);
  // Deletes Rules and puts the driver into passive mode
  void stop();

  // Returns true if the split-tunnel driver is now up and running.
  bool isRunning();

  static bool detectConflict();

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

 private:
  // Installes the Kernel Driver as Driver Service
  static SC_HANDLE installDriver();
  static bool uninstallDriver();
  static bool isInstalled();
  static bool initDriver(HANDLE driverIO);
  static DRIVER_STATE getState(HANDLE driverIO);
  static bool resetDriver(HANDLE driverIO);

  HANDLE m_driver = INVALID_HANDLE_VALUE;
  DRIVER_STATE getState();
  QString stateString();

  // Generates a Configuration for Each APP
  std::vector<uint8_t> generateAppConfiguration(const QStringList& appPaths);
  // Generates a Configuration which IP's are VPN and which network
  std::vector<std::byte> generateIPConfiguration(int inetAdapterIndex, int vpnAdapterIndex = 0);
  std::vector<uint8_t> generateProcessBlob();

  bool registerIPConfiguration(bool force);
  bool startAddressMonitoring();
  void stopAddressMonitoring();
  void scheduleAddressRefresh();
  static void dispatchAddressRefresh(PVOID context);
  static void CALLBACK routeChangeCallback(PVOID context,
                                           PMIB_IPFORWARD_ROW2 row,
                                           MIB_NOTIFICATION_TYPE type);
  static void CALLBACK addressChangeCallback(PVOID context,
                                             PMIB_UNICASTIPADDRESS_ROW row,
                                             MIB_NOTIFICATION_TYPE type);
  static void CALLBACK interfaceChangeCallback(PVOID context,
                                               PMIB_IPINTERFACE_ROW row,
                                               MIB_NOTIFICATION_TYPE type);

  [[nodiscard]] bool getAddress(int adapterIndex, IN_ADDR* out_ipv4,
                                IN6_ADDR* out_ipv6);
  // Collects info about an Opened Process

  // Converts a path to a Dos Path:
  // e.g C:/a.exe -> /harddisk0/a.exe
  QString convertPath(const QString& path);

  struct NotificationContext;
  QTimer m_addressRefreshTimer;
  NotificationContext* m_notificationContext = nullptr;
  HANDLE m_routeChangeHandle = nullptr;
  HANDLE m_addressChangeHandle = nullptr;
  HANDLE m_interfaceChangeHandle = nullptr;
  bool m_addressMonitoringActive = false;
  int m_inetAdapterIndex = 0;
  int m_vpnAdapterIndex = 0;
  std::uint32_t m_addressRefreshRetryAttempts = 0;
  std::vector<std::byte> m_lastIPConfiguration;
};

#endif  // WINDOWSSPLITTUNNEL_H
