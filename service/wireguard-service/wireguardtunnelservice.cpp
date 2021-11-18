#include "wireguardtunnelservice.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <strsafe.h>
#include <iostream>
#include <fstream>
#include <stdint.h>


void debug_log(const std::wstring& msg)
{
    std::wcerr << msg << std::endl;
}

WireguardTunnelService::WireguardTunnelService(const std::wstring& configFile):
    m_configFile{configFile}
{
}

void WireguardTunnelService::addService()
{
    SC_HANDLE scm;
    SC_HANDLE service;
    scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == scm) {
        debug_log(L"OpenSCManager failed");
        return;
    }
    WCHAR szFileName[MAX_PATH];

    GetModuleFileNameW(NULL, szFileName, MAX_PATH);
    std::wstring runCommand = szFileName;
    runCommand += TEXT(" --run ");
    runCommand += m_configFile;

    debug_log(runCommand);
    // check if service is already running
    service = OpenServiceW(
                scm,
                SVCNAME,
                SERVICE_ALL_ACCESS
                );
    if (NULL != service) {
        //service is already running, remove it before add new service
        debug_log(L"service is already running, remove it before add new service");
        CloseServiceHandle(service);
        removeService();
    }
    service = CreateServiceW(
                scm,
                SVCNAME,
                SVCNAME,
                SERVICE_ALL_ACCESS,
                SERVICE_WIN32_OWN_PROCESS,
                SERVICE_DEMAND_START,
                SERVICE_ERROR_NORMAL,
                runCommand.c_str(),
                NULL,
                NULL,
                TEXT("Nsi\0TcpIp"),
                NULL,
                NULL);
    if (NULL == service) {
        debug_log(L"CreateServiceW failed");
        CloseServiceHandle(scm);
        return;
    }
    SERVICE_SID_INFO info;
    info.dwServiceSidType = SERVICE_SID_TYPE_UNRESTRICTED;
    if (ChangeServiceConfig2W(service,
                              SERVICE_CONFIG_SERVICE_SID_INFO,
                              &info) == 0) {
        debug_log(L"ChangeServiceConfig2 failed");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return;
    }
    if (StartServiceW(service, 0, NULL) == 0) {
        debug_log(L"StartServiceW failed");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return;
    }
    if (DeleteService(service) == 0) {
        debug_log(L"DeleteService failed");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}

void WireguardTunnelService::removeService()
{
    SC_HANDLE scm;
    SC_HANDLE service;
    scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == scm) {
        debug_log(L"OpenSCManager failed");
        return;
    }
    service = OpenServiceW(
                scm,
                SVCNAME,
                SERVICE_ALL_ACCESS
                );
    if (NULL == service) {
        debug_log(L"OpenServiceW failed");
        CloseServiceHandle(scm);
        return;
    }
    SERVICE_STATUS stt;
    if (ControlService(service, SERVICE_CONTROL_STOP, &stt) == 0) {
        debug_log(L"ControlService failed");
        DeleteService(service);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return;
    }
    for (int i = 0;
         i < 180 && QueryServiceStatus(scm, &stt) && stt.dwCurrentState != SERVICE_STOPPED;
         ++i) {
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
    DeleteService(service);
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}


int WireguardTunnelService::startTunnel()
{
    debug_log(TEXT(__FUNCTION__));

    HMODULE tunnelLib = LoadLibrary(TEXT("tunnel.dll"));
    if (!tunnelLib) {
        debug_log(L"Failed to load tunnel.dll");
        return 1;
    }

    typedef bool WireGuardTunnelService(const LPCWSTR settings);

    WireGuardTunnelService* tunnelProc = (WireGuardTunnelService*)GetProcAddress(
                tunnelLib, "WireGuardTunnelService");
    if (!tunnelProc) {
        debug_log(L"Failed to get WireGuardTunnelService function");
        return 1;
    }

    debug_log(m_configFile.c_str());

    if (!tunnelProc(m_configFile.c_str())) {
        debug_log(L"Failed to activate the tunnel service");
        return 1;
    }
    return 0;
}

