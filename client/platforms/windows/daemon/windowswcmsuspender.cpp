/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowswcmsuspender.h"

#include <windows.h>
#include <winsvc.h>

#include "logger.h"

#pragma comment(lib, "advapi32.lib")

namespace {
Logger logger("WcmSuspender");

constexpr LONG kNtStatusSuccess = 0;

typedef LONG(NTAPI* NtProcessFn)(HANDLE ProcessHandle);

struct NtFunctions {
  NtProcessFn suspend = nullptr;
  NtProcessFn resume = nullptr;
};

const NtFunctions& ntFunctions() {
  static const NtFunctions fns = [] {
    NtFunctions out;
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
      out.suspend = reinterpret_cast<NtProcessFn>(
          GetProcAddress(ntdll, "NtSuspendProcess"));
      out.resume = reinterpret_cast<NtProcessFn>(
          GetProcAddress(ntdll, "NtResumeProcess"));
    }
    if (!out.suspend || !out.resume) {
      out = NtFunctions();
    }
    return out;
  }();
  return fns;
}

// Required to open wcmSvc; LocalSystem has it, but disabled.
void enableDebugPrivilege() {
  static const bool done = [] {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
                          &token)) {
      return true;
    }
    TOKEN_PRIVILEGES priv = {};
    if (LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege",
                              &priv.Privileges[0].Luid)) {
      priv.PrivilegeCount = 1;
      priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
      AdjustTokenPrivileges(token, FALSE, &priv, sizeof(priv), nullptr,
                            nullptr);
    }
    CloseHandle(token);
    return true;
  }();
  Q_UNUSED(done);
}

DWORD servicePid(LPCWSTR serviceName) {
  SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm) {
    return 0;
  }
  DWORD pid = 0;
  SC_HANDLE svc = OpenServiceW(scm, serviceName, SERVICE_QUERY_STATUS);
  if (svc) {
    SERVICE_STATUS_PROCESS ssp = {};
    DWORD bytesNeeded = 0;
    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                             reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp),
                             &bytesNeeded)) {
      pid = ssp.dwProcessId;
    }
    CloseServiceHandle(svc);
  }
  CloseServiceHandle(scm);
  return pid;
}
}  // namespace

WcmSuspender::WcmSuspender(int pendingRouteOps) {
  if (pendingRouteOps < kThreshold) {
    return;
  }
  const NtFunctions& nt = ntFunctions();
  if (!nt.suspend) {
    return;
  }
  enableDebugPrivilege();

  DWORD pid = servicePid(L"wcmSvc");
  if (pid == 0) {
    return;
  }
  m_process = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
  if (!m_process) {
    return;
  }
  if (nt.suspend(m_process) == kNtStatusSuccess) {
    m_suspended = true;
    logger.debug() << "wcmSvc suspended for" << pendingRouteOps
                   << "route operations";
  } else {
    CloseHandle(m_process);
    m_process = nullptr;
  }
}

WcmSuspender::~WcmSuspender() {
  if (!m_process) {
    return;
  }
  if (m_suspended) {
    ntFunctions().resume(m_process);
    logger.debug() << "wcmSvc resumed";
  }
  CloseHandle(m_process);
}
