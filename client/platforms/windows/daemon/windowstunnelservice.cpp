/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowstunnelservice.h"

#include <Windows.h>
#include <sddl.h>

#include <QDateTime>
#include <QRandomGenerator>
#include <QScopeGuard>

#include <atomic>
#include <thread>

#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/windowscommons.h"
#include "platforms/windows/windowsutils.h"
#include "windowsdaemon.h"

#define TUNNEL_NAMED_PIPE TUNNEL_PIPE_PREFIX "AmneziaVPN"

constexpr uint32_t WINDOWS_TUNNEL_MONITOR_TIMEOUT_MSEC = 2000;
constexpr uint32_t WINDOWS_TUNNEL_CONFIG_TIMEOUT_MSEC = 15000;
constexpr const wchar_t* WINDOWS_TUNNEL_CONFIG_PIPE_SDDL = L"D:P(A;;GA;;;SY)";

namespace {
Logger logger("WindowsTunnelService");
}  // namespace

static bool stopAndDeleteTunnelService(SC_HANDLE service);
static bool waitForServiceStatus(SC_HANDLE service, DWORD expectedStatus);
static HANDLE createConfigPipe(const QString& pipeName);
static bool writeConfigToPipe(HANDLE pipe, const QString& configData);

WindowsTunnelService::WindowsTunnelService(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(WindowsTunnelService);
  logger.debug() << "WindowsTunnelService created.";

  m_scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
  if (m_scm == nullptr) {
    WindowsUtils::windowsLog("Failed to open SCManager");
  }

  // Is the service already running? Terminate it.
  SC_HANDLE service =
      OpenService((SC_HANDLE)m_scm, TUNNEL_SERVICE_NAME, SERVICE_ALL_ACCESS);
  if (service != nullptr) {
    logger.info() << "Tunnel already exists. Terminating it.";
    stopAndDeleteTunnelService(service);
    CloseServiceHandle(service);
  }

  connect(&m_timer, &QTimer::timeout, this, &WindowsTunnelService::timeout);
}

WindowsTunnelService::~WindowsTunnelService() {
  MZ_COUNT_CTOR(WindowsTunnelService);
  stop();
  CloseServiceHandle((SC_HANDLE)m_scm);
}

void WindowsTunnelService::stop() {
  SC_HANDLE service = (SC_HANDLE)m_service;
  if (service) {
    stopAndDeleteTunnelService(service);
    CloseServiceHandle(service);
    m_service = nullptr;
  }

  m_timer.stop();

  if (m_logworker) {
    m_logthread.quit();
    m_logthread.wait();
    m_logworker = nullptr;
  }
}

bool WindowsTunnelService::isRunning() {
  if (m_service == nullptr) {
    return false;
  }

  SERVICE_STATUS status;
  if (!QueryServiceStatus((SC_HANDLE)m_service, &status)) {
    return false;
  }

  return status.dwCurrentState == SERVICE_RUNNING;
}

void WindowsTunnelService::timeout() {
  if (m_service == nullptr) {
    logger.error() << "The service doesn't exist";
    emit backendFailure();
    return;
  }

  SERVICE_STATUS status;
  if (!QueryServiceStatus((SC_HANDLE)m_service, &status)) {
    WindowsUtils::windowsLog("Failed to retrieve the service status");
    emit backendFailure();
    return;
  }

  if (status.dwCurrentState == SERVICE_RUNNING) {
    // The service is active
    return;
  }

  logger.debug() << "The service is not active";
  emit backendFailure();
}

bool WindowsTunnelService::start(const QString& configData) {
  logger.debug() << "Starting the tunnel service";

  m_logworker = new WindowsTunnelLogger(WindowsCommons::tunnelLogFile());
  m_logworker->moveToThread(&m_logthread);
  connect(&m_logthread, &QThread::finished, m_logworker, &QObject::deleteLater);
  m_logthread.start();

  SC_HANDLE scm = (SC_HANDLE)m_scm;
  SC_HANDLE service = nullptr;
  HANDLE configPipe = INVALID_HANDLE_VALUE;
  auto guard = qScopeGuard([&] {
    if (configPipe != INVALID_HANDLE_VALUE) {
      CloseHandle(configPipe);
    }
    if (service) {
      CloseServiceHandle(service);
    }
    m_logthread.quit();
    m_logthread.wait();
    delete m_logworker;
    m_logworker = nullptr;
  });

  // Let's see if we have to delete a previous instance.
  service = OpenService(scm, TUNNEL_SERVICE_NAME, SERVICE_ALL_ACCESS);
  if (service) {
    logger.debug() << "An existing service has been detected. Let's close it.";
    if (!stopAndDeleteTunnelService(service)) {
      return false;
    }
    CloseServiceHandle(service);
    service = nullptr;
  }

  const QString pipeName =
      QString(TUNNEL_CONFIG_PIPE_PREFIX) +
      QString::number(QRandomGenerator::system()->generate64(), 16) +
      QString::number(QRandomGenerator::system()->generate64(), 16);

  configPipe = createConfigPipe(pipeName);
  if (configPipe == INVALID_HANDLE_VALUE) {
    return false;
  }

  QString serviceCmdline;
  {
    QTextStream out(&serviceCmdline);
    out << "\"" << qApp->applicationFilePath() << "\" tunneldaemon \""
        << pipeName << "\"";
  }

  logger.debug() << "Service:" << qApp->applicationFilePath();

  service = CreateService(scm, TUNNEL_SERVICE_NAME, L"Amnezia VPN (tunnel)",
                          SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                          (const wchar_t*)serviceCmdline.utf16(), nullptr, 0,
                          TEXT("Nsi\0TcpIp\0"), nullptr, nullptr);
  if (!service) {
    WindowsUtils::windowsLog("Failed to create the tunnel service");
    return false;
  }

  SERVICE_DESCRIPTION sd = {
      (wchar_t*)L"Manages the Amnezia VPN tunnel connection"};

  if (!ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &sd)) {
    WindowsUtils::windowsLog(
        "Failed to set the description to the tunnel service");
    return false;
  }

  SERVICE_SID_INFO ssi;
  ssi.dwServiceSidType = SERVICE_SID_TYPE_UNRESTRICTED;
  if (!ChangeServiceConfig2(service, SERVICE_CONFIG_SERVICE_SID_INFO, &ssi)) {
    WindowsUtils::windowsLog("Failed to set the SID to the tunnel service");
    return false;
  }

  std::atomic<bool> started(false);
  std::thread starter(
      [&] { started = (StartService(service, 0, nullptr) != FALSE); });

  const bool configSent = writeConfigToPipe(configPipe, configData);
  starter.join();

  CloseHandle(configPipe);
  configPipe = INVALID_HANDLE_VALUE;

  if (!configSent) {
    logger.error() << "Failed to hand the tunnel config over to the service";
    return false;
  }

  if (!started) {
    WindowsUtils::windowsLog("Failed to start the service");
    return false;
  }

  if (waitForServiceStatus(service, SERVICE_RUNNING)) {
    logger.debug() << "The tunnel service is up and running";
    guard.dismiss();
    m_service = service;
    m_timer.start(WINDOWS_TUNNEL_MONITOR_TIMEOUT_MSEC);
    return true;
  }

  logger.error() << "Failed to run the tunnel service";

  SERVICE_STATUS status;
  if (!QueryServiceStatus(service, &status)) {
    WindowsUtils::windowsLog("Failed to retrieve the service status");
    return false;
  }

  logger.debug() << "The tunnel service exited with status:"
                 << status.dwWin32ExitCode << "-" << exitCodeToFailure(&status);

  emit backendFailure();
  return false;
}

static HANDLE createConfigPipe(const QString& pipeName) {
  PSECURITY_DESCRIPTOR descriptor = nullptr;
  if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
          WINDOWS_TUNNEL_CONFIG_PIPE_SDDL, SDDL_REVISION_1, &descriptor,
          nullptr)) {
    WindowsUtils::windowsLog("Failed to build the config pipe descriptor");
    return INVALID_HANDLE_VALUE;
  }
  auto guard = qScopeGuard([&] { LocalFree(descriptor); });

  SECURITY_ATTRIBUTES attributes;
  attributes.nLength = sizeof(attributes);
  attributes.lpSecurityDescriptor = descriptor;
  attributes.bInheritHandle = FALSE;

  HANDLE pipe = CreateNamedPipeW(
      (const wchar_t*)pipeName.utf16(),
      PIPE_ACCESS_OUTBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 64 * 1024, 0,
      0, &attributes);
  if (pipe == INVALID_HANDLE_VALUE) {
    WindowsUtils::windowsLog("Failed to create the config pipe");
  }

  return pipe;
}

static bool waitForOverlapped(HANDLE pipe, OVERLAPPED* overlapped,
                              DWORD* transferred) {
  if (WaitForSingleObject(overlapped->hEvent,
                          WINDOWS_TUNNEL_CONFIG_TIMEOUT_MSEC) != WAIT_OBJECT_0) {
    CancelIo(pipe);
    return false;
  }

  return GetOverlappedResult(pipe, overlapped, transferred, FALSE);
}

static bool writeConfigToPipe(HANDLE pipe, const QString& configData) {
  OVERLAPPED overlapped;
  SecureZeroMemory(&overlapped, sizeof(overlapped));
  overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (overlapped.hEvent == nullptr) {
    WindowsUtils::windowsLog("Failed to create the config pipe event");
    return false;
  }
  auto guard = qScopeGuard([&] { CloseHandle(overlapped.hEvent); });

  DWORD transferred = 0;
  if (!ConnectNamedPipe(pipe, &overlapped)) {
    DWORD error = GetLastError();
    if (error == ERROR_IO_PENDING) {
      if (!waitForOverlapped(pipe, &overlapped, &transferred)) {
        WindowsUtils::windowsLog("The tunnel process never claimed its config");
        return false;
      }
    } else if (error != ERROR_PIPE_CONNECTED) {
      WindowsUtils::windowsLog("Failed to accept the tunnel process");
      return false;
    }
  }

  const QByteArray payload = configData.toUtf8();
  ResetEvent(overlapped.hEvent);
  if (!WriteFile(pipe, payload.constData(), payload.size(), nullptr,
                 &overlapped)) {
    if (GetLastError() != ERROR_IO_PENDING) {
      WindowsUtils::windowsLog("Failed to write the config into the pipe");
      return false;
    }
    if (!waitForOverlapped(pipe, &overlapped, &transferred)) {
      WindowsUtils::windowsLog("Timed out writing the config into the pipe");
      return false;
    }
  }

  FlushFileBuffers(pipe);
  DisconnectNamedPipe(pipe);
  return true;
}

QString WindowsTunnelService::readConfigFromPipe(const QString& pipeName) {
  LPCWSTR name = (const wchar_t*)pipeName.utf16();
  HANDLE pipe = CreateFileW(name, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0,
                            nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    if (!WaitNamedPipeW(name, WINDOWS_TUNNEL_CONFIG_TIMEOUT_MSEC)) {
      WindowsUtils::windowsLog("Failed to wait for the config pipe");
      return QString();
    }
    pipe = CreateFileW(name, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0,
                       nullptr);
  }
  if (pipe == INVALID_HANDLE_VALUE) {
    WindowsUtils::windowsLog("Failed to open the config pipe");
    return QString();
  }
  auto guard = qScopeGuard([&] { CloseHandle(pipe); });

  QByteArray payload;
  while (true) {
    char buffer[512];
    DWORD read = 0;
    if (!ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) || read == 0) {
      break;
    }
    payload.append(buffer, read);
  }

  return QString::fromUtf8(payload);
}

static bool stopAndDeleteTunnelService(SC_HANDLE service) {
  SERVICE_STATUS status;
  if (!QueryServiceStatus(service, &status)) {
    WindowsUtils::windowsLog("Failed to retrieve the service status");
    return false;
  }

  logger.debug() << "The current service is stopped:"
                 << (status.dwCurrentState == SERVICE_STOPPED);

  if (status.dwCurrentState != SERVICE_STOPPED) {
    logger.debug() << "The service is not stopped yet.";
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
      WindowsUtils::windowsLog("Failed to control the service");
      return false;
    }

    if (!waitForServiceStatus(service, SERVICE_STOPPED)) {
      logger.error() << "Unable to stop the service";
      return false;
    }
  }

  logger.debug() << "Proceeding with the deletion";

  if (!DeleteService(service)) {
    WindowsUtils::windowsLog("Failed to delete the service");
    return false;
  }

  return true;
}

QString WindowsTunnelService::uapiCommand(const QString& command) {
  // Create a pipe to the tunnel service.
  LPTSTR tunnelName = (LPTSTR)TEXT(TUNNEL_NAMED_PIPE);
  HANDLE pipe = CreateFile(tunnelName, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    return QString();
  }

  auto guard = qScopeGuard([&] { CloseHandle(pipe); });
  if (!WaitNamedPipe(tunnelName, 1000)) {
    WindowsUtils::windowsLog("Failed to wait for named pipes");
    return QString();
  }

  DWORD mode = PIPE_READMODE_BYTE;
  if (!SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr)) {
    WindowsUtils::windowsLog("Failed to set the read-mode on pipe");
    return QString();
  }

  // Write the UAPI command into the pipe.
  QByteArray message = command.toLocal8Bit();
  DWORD written;
  while (!message.endsWith("\n\n")) {
    message.append('\n');
  }
  if (!WriteFile(pipe, message.constData(), message.length(), &written,
                 nullptr)) {
    WindowsUtils::windowsLog("Failed to write into the pipe");
    return QString();
  }

  // Receive the response from the pipe.
  QByteArray reply;
  while (!reply.contains("\n\n")) {
    char buffer[512];
    DWORD read = 0;
    if (!ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr)) {
      break;
    }

    reply.append(buffer, read);
  }

  return QString::fromUtf8(reply).trimmed();
}

// static
static bool waitForServiceStatus(SC_HANDLE service, DWORD expectedStatus) {
  int tries = 0;
  while (tries < 30) {
    SERVICE_STATUS status;
    if (!QueryServiceStatus(service, &status)) {
      WindowsUtils::windowsLog("Failed to retrieve the service status");
      return false;
    }

    if (status.dwCurrentState == expectedStatus) {
      return true;
    }

    logger.warning() << "The service is not in the right status yet.";

    Sleep(1000);
    ++tries;
  }

  return false;
}

// static
QString WindowsTunnelService::exitCodeToFailure(const void* status) {
  const SERVICE_STATUS* st = static_cast<const SERVICE_STATUS*>(status);
  if (st->dwWin32ExitCode != ERROR_SERVICE_SPECIFIC_ERROR) {
    return WindowsUtils::getErrorMessage(st->dwWin32ExitCode);
  }

  // The order of this error code is taken from wireguard.
  switch (st->dwServiceSpecificExitCode) {
    case 0:
      return "No error";
    case 1:
      return "Error when opening the ringlogger log file";
    case 2:
      return "Error while loading the WireGuard configuration file from "
             "path.";
    case 3:
      return "Error while creating a WinTun device.";
    case 4:
      return "Error while listening on a named pipe.";
    case 5:
      return "Error while resolving DNS hostname endpoints.";
    case 6:
      return "Error while manipulating firewall rules.";
    case 7:
      return "Error while setting the device configuration.";
    case 8:
      return "Error while binding sockets to default routes.";
    case 9:
      return "Unable to set interface addresses, routes, dns, and/or "
             "interface settings.";
    case 10:
      return "Error while determining current executable path.";
    case 11:
      return "Error while opening the NUL file.";
    case 12:
      return "Error while attempting to track tunnels.";
    case 13:
      return "Error while attempting to enumerate current sessions.";
    case 14:
      return "Error while dropping privileges.";
    case 15:
      return "Windows internal error.";
    default:
      return QString("Unknown error (%1)").arg(st->dwServiceSpecificExitCode);
  }
}
