#include "winhelp.h"

#include <stdio.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <Ras.h>
#include <iphlpapi.h>


#define REG_AUTORUN_PATH  "Software\\Microsoft\\Windows\\CurrentVersion\\Run"

int winhelpGetRegistry(const char *name, const char *reg, char *value)
{
    unsigned long nType = REG_SZ, nData = MAX_PATH;
    if(ERROR_SUCCESS != SHGetValueA(HKEY_CURRENT_USER, reg,
        name, &nType, value, &nData))
        return -1;
    return 1;
}

int winhelpSetRegistry(const char *name, const char *reg, const char *value)
{
    if(ERROR_SUCCESS != SHSetValueA(HKEY_CURRENT_USER, reg,
        name, REG_SZ, value, (DWORD)strlen(value)))
        return -1;
    return 1;
}

int winhelpLaunchStartupRegister(const char *name, int enable, const char *p)
{
    char path[MAX_PATH] = {0};
    if(p && strlen(p) == 0)
        p = NULL;
    if(p) {
        if(enable)
            strcpy(path, "\"");
        else
            strcpy(path, ";\"");
        GetModuleFileNameA(NULL, path + strlen(path), MAX_PATH);
        strcat(path, "\" ");
        strcat(path, p);
    } else {
        if(enable)
            strcpy(path, "");
        else
            strcpy(path, ";");
        GetModuleFileNameA(NULL, path + strlen(path), MAX_PATH);
    }
    if(winhelpSetRegistry(name, REG_AUTORUN_PATH, path) < 0)
        return -1;
    return 1;
}

/* use the task scheduler, we do not need to care about UAC when start up */
int winhelpLaunchStartupTaskScheduler(const char *name, int enable, const char *p)
{
    char cmd[MAX_PATH * 10] = {0};
    char path[MAX_PATH] = {0};
    UINT i = 0;
    GetModuleFileNameA(NULL, path, MAX_PATH);
    if (QString(path).contains("build-vpn-")) {
        qDebug() << "winhelpLaunchStartupTaskScheduler : skipping auto launch for build dir";
        return 0;
    }

    if(enable) {
        if(p == NULL)
            p = "";
        sprintf(cmd, "schtasks /create /sc onlogon /tr \"\\\"%s\\\" %s\" "
            "/tn \"%s\" /f /rl highest", path, p, name);

    } else {
        sprintf(cmd, "schtasks /delete /tn \"%s\" /f", name);
    }
    qDebug().noquote() << "winhelpLaunchStartupTaskScheduler cmd:" << cmd;
    i = WinExec(cmd, SW_HIDE);
    return 1;
}

int winhelpLaunchStartup(const char *name, int enable, const char *p)
{
    OSVERSIONINFOA info = {0};
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if(GetVersionExA(&info) < 0)
        return -2;
    if(info.dwMajorVersion >= 6)
        return winhelpLaunchStartupTaskScheduler(name, enable, p);
    else
        return winhelpLaunchStartupRegister(name, enable, p);
}

int str2int(const char *s)
{
    int r = 0;
    while(*s && *(s + 1) && *(s + 2) && *(s + 3)) {
        r += ((const int *)s)[0];
        s += 4;
    }
    return r;
}

int winhelpOneProcess()
{
    char path[MAX_PATH] = {0};
    int i = 0, size = 0, cur = 0;
    GetModuleFileNameA(NULL, path, MAX_PATH);
    size = strlen(path);
    for(i = size; i >= 0 && path[i] != '\\' ; i--);
    cur = i + 1;
    while(path[i] != '.' && path[i])i++;
    path[i] = '\0';
    sprintf(path, "ONE_%s", path + cur);
    CreateEventA(NULL, FALSE, FALSE, path);
    if(GetLastError())
        return 0;
    return 1;
}

int winhelpSystemBits()
{
    typedef BOOL (WINAPI *LPFN_ISWOW64)(HANDLE, PBOOL);
    int b64 = FALSE;
    LPFN_ISWOW64 fnIsWow64 = (LPFN_ISWOW64)GetProcAddress(
        GetModuleHandleA("kernel32"), "IsWow64Process");
    if(NULL != fnIsWow64) {
        if(!fnIsWow64(GetCurrentProcess(),&b64)) {
            return -1;
        }
    }
    return b64 ? 64 : 86;
}

bool winhelpIsSystem_x64()
{
    typedef BOOL (WINAPI *LPFN_ISWOW64)(HANDLE, PBOOL);
    int b64 = FALSE;
    LPFN_ISWOW64 fnIsWow64 = (LPFN_ISWOW64)GetProcAddress(
        GetModuleHandleA("kernel32"), "IsWow64Process");
    if(NULL != fnIsWow64) {
        if(!fnIsWow64(GetCurrentProcess(),&b64)) {
            return false;
        }
    }
    return b64 ? true : false;
}

typedef struct _PROCESS_QUERY
{
    HANDLE h;
    PROCESSENTRY32 d;
}PROCESS_QUERY;

int winhelpProcessQuery(void **p)
{
    PROCESS_QUERY *q = (PROCESS_QUERY *)malloc(sizeof(PROCESS_QUERY));
    if(q == NULL)
        return -2;
    q->h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(q->h == INVALID_HANDLE_VALUE) {
        free(q);
        return -1;
    }
    if(FALSE == Process32First(q->h, &q->d)) {
        CloseHandle(q->h);
        free(q);
        return -3;
    }
    *p = (void *)q;
    return 1;
}

int winhelpProcessNext(void *h, char *name)
{
    PROCESS_QUERY *q = (PROCESS_QUERY *)h;
    int pid = 0;
    if(q == NULL)
        return 0;
    if(q->h == NULL) {
        free(q);
        return 0;
    }
    if(name)
        wsprintfA(name, "%ls", q->d.szExeFile);
    pid = (int)q->d.th32ProcessID;

    if(FALSE == Process32Next(q->h, &q->d)) {
        CloseHandle(q->h);
        q->h = NULL;
    }
    return pid;
}

int winhelpRecvEvent(const char *event)
{
    HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, event);
    if(hEvent == NULL)
        return 0;
    WaitForSingleObject(hEvent, INFINITE);
    CloseHandle(hEvent);
    return 1;
}

int winhelpSendEvent(const char *event)
{
    HANDLE hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, event);
    if(hEvent == NULL)
        return 0;
    SetEvent(hEvent);
    CloseHandle(hEvent);
    return 1;
}

int winhelpSystemVersion()
{
    OSVERSIONINFOA ver = {0};
    int version = 0;
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&ver);
    version = ver.dwMajorVersion * 0x100 + ver.dwMinorVersion;
    return version;
}

int winhelperSetMTUSize(const char *subname, int size)
{
    char cmd[MAX_PATH] = {0};
    sprintf(cmd, "netsh interface ipv4 set subinterface \"%s\" mtu=%d store=persistent",
        subname, size);
    return (int)WinExec(cmd, SW_HIDE);
}

int winhelpRecvSendBytes(const char *dev, int *recv, int *send)
{
    MIB_IFTABLE *it = NULL;
    DWORD        size = sizeof(MIB_IFTABLE), ret = 0, i = 0;
    DWORD        tr = 0, ts = 0;

    if(it = (MIB_IFTABLE *)malloc(sizeof (MIB_IFTABLE)), it == NULL)
        return -1;
    if(GetIfTable(it, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        free(it);
        if(it = (MIB_IFTABLE *)malloc(size), it == NULL)
            return -2;
    }
    if(ret = GetIfTable(it, &size, FALSE), ret != NO_ERROR) {
        free(it);
        return -3;
    }
    for(i = 0; i < it->dwNumEntries; i++) {
        MIB_IFROW *ir = &it->table[i];
        if(strstr((const char *)ir->bDescr, dev) == 0)
            continue;
        tr += (int)ir->dwInOctets;
        ts += (int)ir->dwOutOctets;
    }
    *recv = tr;
    *send = ts;
    free(it);
    return 1;
}

