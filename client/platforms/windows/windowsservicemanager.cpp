/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsservicemanager.h"

#include <QApplication>
#include <QTimer>

#include "Windows.h"
#include "Winsvc.h"
#include "logger.h"
//#include "mozillavpn.h"
#include "platforms/windows/windowsutils.h"

namespace {
Logger logger("WindowsServiceManager");
}

WindowsServiceManager::WindowsServiceManager(SC_HANDLE serviceManager,
                                             SC_HANDLE service)
    : QObject(qApp), m_serviceManager(serviceManager), m_service(service) {
  m_timer.setSingleShot(false);
}

std::unique_ptr<WindowsServiceManager> WindowsServiceManager::open(
    const QString serviceName) {
  LPCWSTR service = (const wchar_t*)serviceName.utf16();

  DWORD err = NULL;
  auto scm_rights = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE |
                    SC_MANAGER_QUERY_LOCK_STATUS | STANDARD_RIGHTS_READ;
  auto manager = OpenSCManager(NULL,  // local computer
                               NULL,  // servicesActive database
                               scm_rights);
  err = GetLastError();
  if (err != NULL) {
    logger.error() << " OpenSCManager failed code: " << err;
    return {};
  }
  logger.debug() << "OpenSCManager access given - " << err;

  logger.debug() << "Opening Service - " << serviceName;
  // Try to get an elevated handle
  auto serviceHandle =
      OpenService(manager,  // SCM database
                  service,  // name of service
                  (GENERIC_READ | SERVICE_START | SERVICE_STOP));
  err = GetLastError();
  if (err != NULL) {
    CloseServiceHandle(manager);
    WindowsUtils::windowsLog("OpenService failed");
    return {};
  }

  logger.debug() << "Service manager execute access granted";
  return std::make_unique<WindowsServiceManager>(manager, serviceHandle);
}

WindowsServiceManager::~WindowsServiceManager() {
  if (m_service != NULL) {
    CloseServiceHandle(m_service);
  }
  if (m_serviceManager != NULL) {
    CloseServiceHandle(m_serviceManager);
  }
}

bool WindowsServiceManager::startPolling(DWORD goal_state, int max_wait_sec) {
  int tries = 0;
  while (tries < max_wait_sec) {
    SERVICE_STATUS status;
    if (!QueryServiceStatus(m_service, &status)) {
      WindowsUtils::windowsLog("Failed to retrieve the service status");
      return false;
    }

    if (status.dwCurrentState == goal_state) {
      if (status.dwCurrentState == SERVICE_RUNNING) {
        emit serviceStarted();
      }
      if (status.dwCurrentState == SERVICE_STOPPED) {
        emit serviceStopped();
      }
      return true;
    }

    logger.debug() << "Polling Status" << m_state_target
                   << "wanted, has: " << status.dwCurrentState;
    Sleep(1000);
    ++tries;
  }
  return false;
}

SERVICE_STATUS_PROCESS WindowsServiceManager::getStatus() {
  SERVICE_STATUS_PROCESS serviceStatus;
  DWORD dwBytesNeeded;  // Contains missing bytes if struct is too small?
  QueryServiceStatusEx(m_service,                       // handle to service
                       SC_STATUS_PROCESS_INFO,          // information level
                       (LPBYTE)&serviceStatus,          // address of structure
                       sizeof(SERVICE_STATUS_PROCESS),  // size of structure
                       &dwBytesNeeded);
  return serviceStatus;
}

bool WindowsServiceManager::startService() {
  auto state = getStatus().dwCurrentState;
  if (state != SERVICE_STOPPED && state != SERVICE_STOP_PENDING) {
    logger.warning() << ("Service start not possible, as its running");
    emit serviceStarted();
    return true;
  }

  bool ok = StartService(m_service,  // handle to service
                         0,          // number of arguments
                         NULL);      // no arguments
  if (ok) {
    logger.debug() << ("Service start requested");
    startPolling(SERVICE_RUNNING, 30);
  } else {
    WindowsUtils::windowsLog("StartService failed");
  }
  return ok;
}

bool WindowsServiceManager::stopService() {
  auto state = getStatus().dwCurrentState;
  if (state != SERVICE_RUNNING && state != SERVICE_START_PENDING) {
    logger.warning() << ("Service stop not possible, as its not running");
  }

  bool ok = ControlService(m_service, SERVICE_CONTROL_STOP, NULL);
  if (ok) {
    logger.debug() << ("Service stop requested");
    startPolling(SERVICE_STOPPED, 10);
  } else {
    WindowsUtils::windowsLog("StopService failed");
  }
  return ok;
}
