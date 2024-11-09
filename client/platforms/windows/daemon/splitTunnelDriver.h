#pragma once
#include <Windows.h>

#include <QStringList>

#include "winHandle.h"


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

#define FILE_ANY_ACCESS 0x0000

#define METHOD_BUFFERED 0
#define METHOD_IN_DIRECT 1
#define METHOD_NEITHER 3

#define CTL_CODE(DeviceType, Function, Method, Access)                                                                 \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// Known ControlCodes
#define IOCTL_INITIALIZE CTL_CODE(0x8000, 1, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_DEQUEUE_EVENT CTL_CODE(0x8000, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_REGISTER_PROCESSES CTL_CODE(0x8000, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_REGISTER_IP_ADDRESSES CTL_CODE(0x8000, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_IP_ADDRESSES CTL_CODE(0x8000, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_CONFIGURATION CTL_CODE(0x8000, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_CONFIGURATION CTL_CODE(0x8000, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CLEAR_CONFIGURATION CTL_CODE(0x8000, 8, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_GET_STATE CTL_CODE(0x8000, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_QUERY_PROCESS CTL_CODE(0x8000, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ST_RESET CTL_CODE(0x8000, 11, METHOD_NEITHER, FILE_ANY_ACCESS)


class WinSplitTunnelDriver {
public:
    WinSplitTunnelDriver() = default;

    enum class InitError {
        None,
        AlreadyInitialized,
        DriverNotFound,
        ConnectionAccessDenied,
        ConnectionFailed,
        ReconfigFailed,
    };

    static bool CheckLoaded();
    InitError init();

    bool reconfigureDriver(int inetAdapterIndex, int vpnAdapterIndex);

    bool initializeDriver();
    bool resetDriver();
    bool setAdapters(int inetAdapterIndex, int vpnAdapterIndex);

    bool registerProcesses();

    bool setConfig(const QStringList &appPaths);
    void clearConfig();

    DRIVER_STATE driverState();

private:
    static bool OpenDriver(WinHandle *outHandle);

private:
    WinHandle m_driver;
};
