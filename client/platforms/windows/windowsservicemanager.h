/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSSERVICEMANAGER
#define WINDOWSSERVICEMANAGER

#include <QObject>
#include <QTimer>

#include "Windows.h"
#include "Winsvc.h"

/**
 * @brief The WindowsServiceManager provides controll over the MozillaVPNBroker
 * service via SCM
 */
class WindowsServiceManager : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(WindowsServiceManager)

 public:
  WindowsServiceManager(LPCWSTR serviceName);
  ~WindowsServiceManager();

  // true if the Service is running
  bool isRunning() { return getStatus().dwCurrentState == SERVICE_RUNNING; };

  // Starts the service if execute rights are present
  // Starts to poll for serviceStarted
  bool startService();

  // Stops the service if execute rights are present.
  // Starts to poll for serviceStopped
  bool stopService();

 signals:
  // Gets Emitted after the Service moved From SERVICE_START_PENDING to
  // SERVICE_RUNNING
  void serviceStarted();
  void serviceStopped();

 private:
  // Returns the State of the Process:
  // See
  // SERVICE_STOPPED,SERVICE_STOP_PENDING,SERVICE_START_PENDING,SERVICE_RUNNING
  SERVICE_STATUS_PROCESS getStatus();
  bool m_has_access = false;
  LPWSTR m_serviceName;
  SC_HANDLE m_serviceManager;
  SC_HANDLE m_service;  // Service handle with r/w priv.
  DWORD m_state_target;
  int m_currentWaitTime;
  int m_maxWaitTime;
  QTimer m_timer;

  bool startPolling(DWORD goal_state, int maxS);
};

#endif  // WINDOWSSERVICEMANAGER
