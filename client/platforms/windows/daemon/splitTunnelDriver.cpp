#include "splitTunnelDriver.h"
#include "../windowscommons.h"
#include "logger.h"
#include "splitTunnelService.h"

// this must come before all other headers
#include <WS2tcpip.h>

#include <ws2ipdef.h>

#include <Psapi.h>
#include <TlHelp32.h>

#include <QMap>
#include <QNetworkInterface>

#include <vector>

constexpr static const auto DRIVER_SYMBOLIC_NAME = L"\\\\.\\MULLVADSPLITTUNNEL";

namespace {
    Logger logger("WinSplitTunnelDriver");
}

struct ProcessInfo {
    uint32_t pid;
    uint32_t parentPid;
    uint64_t creationTime;
    std::wstring devicePath;
};

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

struct ProcessRegistryHeader {};

QList<ProcessInfo> buildProcessTree();
std::vector<uint8_t> serializeProcessTree(const QList<ProcessInfo> &processes);
std::vector<uint8_t> serializeConfiguration(const QStringList &appPaths);
void getAddress(int adapterIndex, IN_ADDR *outIpv4, IN6_ADDR *outIpv6);

bool OpenDriverInner(WinHandle *outHandle, bool tryAgain = true) {
    WinHandle file =
            CreateFile(DRIVER_SYMBOLIC_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (!file) {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_ACCESS_DENIED && tryAgain) {
            WinSplitTunnelService::KillConflictingServices();
            return OpenDriverInner(outHandle, false);
        }

        qDebug() << "Failed to open driver, error: " << lastError;
        return false;
    }

    if (outHandle) {
        *outHandle = std::move(file);
    }

    return true;
}

bool WinSplitTunnelDriver::OpenDriver(WinHandle *outHandle) { return OpenDriverInner(outHandle); }

bool WinSplitTunnelDriver::CheckLoaded() {
    WinHandle file{};
    if (!OpenDriver(&file))
        return false;
    return file;
}


WinSplitTunnelDriver::InitError WinSplitTunnelDriver::init() {
    if (m_driver)
        return InitError::AlreadyInitialized;

    if (!OpenDriver(&m_driver) || !m_driver) {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND) {
            return InitError::DriverNotFound;
        }

        if (lastError == ERROR_ACCESS_DENIED) {
            return InitError::ConnectionAccessDenied;
        }

        logger.debug() << "Failed to connect to driver, error: " << lastError;
        return InitError::ConnectionFailed;
    }

    return InitError::None;
}

bool WinSplitTunnelDriver::reconfigureDriver(int inetAdapterIndex, int vpnAdapterIndex) {
    DRIVER_STATE state = driverState();
    if (state != STATE_STARTED) {
        if (!resetDriver()) {
            return false;
        }
    }

    logger.debug() << "Reinitializing driver";
    if (!initializeDriver()) {
        return false;
    }

    if (!registerProcesses()) {
        return false;
    }

    if (!setAdapters(inetAdapterIndex, vpnAdapterIndex)) {
        return false;
    }

    return true;
}

bool WinSplitTunnelDriver::initializeDriver() {
    if (!m_driver) {
        logger.debug() << "Tried to initialize a non-opened driver";
        return false;
    }

    bool ok = DeviceIoControl(m_driver, IOCTL_INITIALIZE, nullptr, 0, nullptr, 0, nullptr, nullptr);
    if (!ok) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to initialize driver, error: " << lastError;
        return false;
    }

    return true;
}

bool WinSplitTunnelDriver::resetDriver() {
    if (!m_driver) {
        logger.debug() << "Tried to reset a non-opened driver";
        return false;
    }

    bool ok = DeviceIoControl(m_driver, IOCTL_ST_RESET, nullptr, 0, nullptr, 0, nullptr, nullptr);
    if (!ok) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to reset driver, error: " << lastError;
        return false;
    }

    return true;
}

ProcessInfo getProcessInfo1(HANDLE process, const PROCESSENTRY32W &processMeta) {
    ProcessInfo pi;
    pi.parentPid = processMeta.th32ParentProcessID;
    pi.pid = processMeta.th32ProcessID;
    pi.creationTime = 0;
    pi.devicePath = L"";

    FILETIME creationTime, null_time;
    auto ok = GetProcessTimes(process, &creationTime, &null_time, &null_time, &null_time);
    if (ok) {
        pi.creationTime = static_cast<uint64_t>(creationTime.dwHighDateTime) << 32 |
                          static_cast<uint64_t>(creationTime.dwLowDateTime);
    }
    wchar_t imagepath[MAX_PATH + 1];
    if (K32GetProcessImageFileNameW(process, imagepath, sizeof(imagepath) / sizeof(*imagepath)) != 0) {
        pi.devicePath = imagepath;
    }
    return pi;
}
bool WinSplitTunnelDriver::setAdapters(int inetAdapterIndex, int vpnAdapterIndex) {
    IP_ADDRESSES_CONFIG addressConfig{};

    auto ifaces = QNetworkInterface::allInterfaces();

    if (vpnAdapterIndex == 0) {
        vpnAdapterIndex = WindowsCommons::VPNAdapterIndex();
    }

    getAddress(vpnAdapterIndex, &addressConfig.TunnelIpv4, &addressConfig.TunnelIpv6);
    getAddress(inetAdapterIndex, &addressConfig.InternetIpv4, &addressConfig.InternetIpv6);

    bool ok = DeviceIoControl(m_driver, IOCTL_REGISTER_IP_ADDRESSES, &addressConfig, sizeof(addressConfig), nullptr, 0,
                              nullptr, nullptr);
    if (!ok) {
        DWORD lastError = GetLastError();
        logger.error() << "Faeiled to set adapters, error: " << lastError;
        return false;
    }

    return true;
}


std::vector<uint8_t> generateProcessBlob() {
    // Get a Snapshot of all processes that are running:
    HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot_handle == INVALID_HANDLE_VALUE) {
        // WindowsUtils::windowsLog("Creating Process snapshot failed");
        return std::vector<uint8_t>(0);
    }
    auto cleanup = qScopeGuard([&] { CloseHandle(snapshot_handle); });
    // Load the First Entry, later iterate over all
    PROCESSENTRY32W currentProcess;
    currentProcess.dwSize = sizeof(PROCESSENTRY32W);

    if (FALSE == (Process32First(snapshot_handle, &currentProcess))) {
        // WindowsUtils::windowsLog("Cant read first entry");
    }

    QMap<DWORD, ProcessInfo> processes;

    do {
        auto process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, currentProcess.th32ProcessID);

        if (process_handle == INVALID_HANDLE_VALUE) {
            continue;
        }
        ProcessInfo info = getProcessInfo1(process_handle, currentProcess);
        processes.insert(info.pid, info);
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

    for (const auto &process: process_list) {
        totalStringSize += (process.devicePath.size() * sizeof(wchar_t));
    }
    auto bufferSize =
            sizeof(PROCESS_DISCOVERY_HEADER) + (sizeof(PROCESS_DISCOVERY_ENTRY) * processes.size()) + totalStringSize;

    std::vector<uint8_t> out(bufferSize);

    auto header = reinterpret_cast<PROCESS_DISCOVERY_HEADER *>(&out[0]);
    auto entry = reinterpret_cast<PROCESS_DISCOVERY_ENTRY *>(header + 1);
    auto stringBuffer = reinterpret_cast<uint8_t *>(entry + processes.size());

    SIZE_T currentStringOffset = 0;

    for (const auto &process: process_list) {
        // Wierd DWORD -> Handle Pointer magic.
        entry->ProcessId = (HANDLE) ((size_t) process.pid);
        entry->ParentProcessId = (HANDLE) ((size_t) process.parentPid);

        if (process.devicePath.empty()) {
            entry->ImageNameOffset = 0;
            entry->ImageNameLength = 0;
        } else {
            const auto imageNameLength = process.devicePath.size() * sizeof(wchar_t);

            entry->ImageNameOffset = currentStringOffset;
            entry->ImageNameLength = static_cast<USHORT>(imageNameLength);

            RtlCopyMemory(stringBuffer + currentStringOffset, &process.devicePath[0], imageNameLength);

            currentStringOffset += imageNameLength;
        }
        ++entry;
    }

    header->NumEntries = processes.size();
    header->TotalLength = bufferSize;

    return out;
}

bool WinSplitTunnelDriver::registerProcesses() {
    if (!m_driver) {
        logger.debug() << "Tried to register processes for a non-opened driver";
        return false;
    }

    auto processTree = buildProcessTree();
    if (processTree.isEmpty()) {
        logger.debug() << "Failed to build process tree";
        return false;
    }

    auto serialized = generateProcessBlob();

    bool ok = DeviceIoControl(m_driver, IOCTL_REGISTER_PROCESSES, serialized.data(), serialized.size(), nullptr, 0,
                              nullptr, nullptr);
    if (!ok) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to register processes, error: " << lastError;
        return false;
    }

    return true;
}

bool WinSplitTunnelDriver::setConfig(const QStringList &appPaths) {
    DRIVER_STATE state = driverState();
    if (state != STATE_READY && state != STATE_RUNNING) {
        logger.warning() << "Tried to set config while driver was not in the correct state, current state: " << state;
        return false;
    }

    auto serialized = serializeConfiguration(appPaths);
    if (serialized.empty()) {
        logger.error() << "Failed to serialize configuration";
        return false;
    }

    bool ok = DeviceIoControl(m_driver, IOCTL_SET_CONFIGURATION, serialized.data(), serialized.size(), nullptr, 0,
                              nullptr, nullptr);
    if (!ok) {
        DWORD lastError = GetLastError();
        logger.error() << "Failed to set configuration, error: " << lastError;
        return false;
    }

    return true;
}

void WinSplitTunnelDriver::clearConfig() {
    if (!m_driver) {
        logger.debug() << "Tried to clear config on a non-opened driver";
        return;
    }

    bool ok = DeviceIoControl(m_driver, IOCTL_CLEAR_CONFIGURATION, nullptr, 0, nullptr, 0, nullptr, nullptr);
    if (!ok) {
        logger.error() << "Failed to stop split tunneling";
        return;
    }
}

DRIVER_STATE WinSplitTunnelDriver::driverState() {
    if (!m_driver) {
        logger.debug() << "Tried to get driver state on a non-opened driver";
        return STATE_UNKNOWN;
    }

    DWORD bytesReturned = 0;
    size_t outBuffer = 0;
    bool ok = DeviceIoControl(m_driver, IOCTL_GET_STATE, nullptr, 0, &outBuffer, sizeof(outBuffer), &bytesReturned,
                              nullptr);
    if (!ok) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to get driver state, error: " << lastError;
        return STATE_UNKNOWN;
    }

    if (bytesReturned == 0) {
        logger.debug() << "Driver returned empty response for state";
        return STATE_UNKNOWN;
    }

    return static_cast<DRIVER_STATE>(outBuffer);
}

bool getProcessDevicePath(HANDLE process, std::wstring *outDevicePath) {
    size_t initialCapacity = 512;

    while (true) {
        std::vector<wchar_t> buffer{};
        buffer.resize(initialCapacity, 0);

        size_t written = GetProcessImageFileNameW(process, buffer.data(), buffer.size());
        if (written == 0) {
            DWORD lastError = GetLastError();
            if (lastError == ERROR_INSUFFICIENT_BUFFER) {
                initialCapacity *= 2;
                continue;
            }

            return false;
        }

        if (outDevicePath) {
            *outDevicePath = std::wstring(buffer.data());
        }

        return true;
    }
}

ProcessInfo getProcessInfo(HANDLE process, const PROCESSENTRY32W &processMeta) {
    ProcessInfo info{.pid = processMeta.th32ProcessID,
                     .parentPid = processMeta.th32ParentProcessID,
                     .creationTime = 0,
                     .devicePath = {}};

    FILETIME creationTime{};
    FILETIME nullTime{};
    bool ok = GetProcessTimes(process, &creationTime, &nullTime, &nullTime, &nullTime);
    if (ok) {
        info.creationTime = static_cast<uint64_t>(creationTime.dwHighDateTime) << 32 |
                            static_cast<uint64_t>(creationTime.dwLowDateTime);
    }

    getProcessDevicePath(process, &info.devicePath);

    return info;
}

QList<ProcessInfo> buildProcessTree() {
    WinHandle snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!snapshotHandle) {
        logger.debug() << "Failed to build process tree";
        return {};
    }

    PROCESSENTRY32W currentProcess{.dwSize = sizeof(PROCESSENTRY32W)};

    if (!Process32FirstW(snapshotHandle, &currentProcess)) {
        logger.debug() << "Failed to read first entry";
    }

    QMap<DWORD, ProcessInfo> processes{};

    do {
        WinHandle processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, currentProcess.th32ProcessID);
        if (!processHandle) {
            continue;
        }

        ProcessInfo info = getProcessInfo(processHandle, currentProcess);
        processes.insert(info.pid, info);
    } while (Process32NextW(snapshotHandle, &currentProcess));

    // https://github.com/mullvad/mullvadvpn-app/blob/c2d6eada9179a5aad8a52b8a4e73c9c71ce50c8c/talpid-core/src/split_tunnel/windows/driver.rs#L533
    for (auto &processInfo: processes.values()) {
        if (processInfo.parentPid == 0) {
            continue;
        }

        auto parentInfo = processes.find(processInfo.parentPid);
        if (parentInfo != processes.end()) {
            if (parentInfo->creationTime > processInfo.creationTime) {
                processInfo.parentPid = 0;
            }
        }
    }

    return processes.values();
}

std::vector<uint8_t> serializeProcessTree(const QList<ProcessInfo> &processes) {
    size_t totalStringSize = 0;
    for (const auto &info: processes) {
        totalStringSize += sizeof(wchar_t) * info.devicePath.size();
    }

    size_t totalBufferSize =
            sizeof(PROCESS_DISCOVERY_HEADER) + sizeof(PROCESS_DISCOVERY_ENTRY) * processes.size() + totalStringSize;

    std::vector<uint8_t> buffer{};
    buffer.resize(totalBufferSize, 0);

    auto header = reinterpret_cast<PROCESS_DISCOVERY_HEADER *>(buffer.data());
    auto entry = reinterpret_cast<PROCESS_DISCOVERY_ENTRY *>(header + 1);
    auto stringBuffer = reinterpret_cast<uint8_t *>(entry + processes.size());

    size_t currentStringOffset = 0;

    for (const auto &info: processes) {
        entry->ProcessId = (HANDLE) ((size_t) info.pid);
        entry->ParentProcessId = (HANDLE) ((size_t) info.parentPid);

        if (info.devicePath.empty()) {
            entry->ImageNameLength = 0;
            entry->ImageNameOffset = 0;
        } else {
            const auto imageNameLength = info.devicePath.size() * sizeof(wchar_t);

            entry->ImageNameOffset = currentStringOffset;
            entry->ImageNameLength = static_cast<USHORT>(imageNameLength);

            memcpy(stringBuffer + currentStringOffset, info.devicePath.data(), imageNameLength);
            currentStringOffset += imageNameLength;
        }

        ++entry;
    }

    header->NumEntries = processes.size();
    header->TotalLength = totalBufferSize;

    return buffer;
}

bool devicePathByHandle(HANDLE file, QString *devicePath) {
    size_t bufferSize = GetFinalPathNameByHandleW(file, nullptr, 0, VOLUME_NAME_NT);

    std::vector<wchar_t> buffer{};
    buffer.resize(bufferSize, 0);

    size_t status = GetFinalPathNameByHandleW(file, buffer.data(), bufferSize, VOLUME_NAME_NT);
    if (status == 0) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to get final path name by handle, error: " << lastError;
        return false;
    }

    *devicePath = QString::fromWCharArray(buffer.data(), buffer.size());
    return true;
}

bool queryDosDevice(QString deviceName, QString *dosDevice) {
    size_t initialCapacity = 64;
    while (true) {
        std::vector<wchar_t> newPrefix{};
        newPrefix.resize(initialCapacity, 0);

        DWORD prefixLen = QueryDosDeviceW(qUtf16Printable(deviceName), newPrefix.data(), newPrefix.size());
        if (prefixLen == 0) {
            DWORD lastError = GetLastError();
            if (lastError == ERROR_INSUFFICIENT_BUFFER) {
                initialCapacity *= 2;
                continue;
            }

            logger.debug() << "Failed to query dos device, error: " << lastError;
            return false;
        }

        if (dosDevice) {
            *dosDevice = QString::fromWCharArray(newPrefix.data());
        }
        return true;
    }
}

QString toDevicePath(const QString &path) {
    LPCWSTR utf16Path = reinterpret_cast<LPCWSTR>(path.utf16());
    WinHandle file = CreateFile(utf16Path, GENERIC_READ, 0, nullptr, 0, 0, nullptr);
    if (file) {
        QString devicePath{};
        if (devicePathByHandle(file, &devicePath)) {
            return devicePath;
        }
    }

    QStringList parts = path.split("/");
    QString driveLetter = parts.takeFirst();
    if (!driveLetter.contains(":") || parts.empty()) {
        // device should contain :, e.g. C:
        return "";
    }

    QString deviceName{};
    if (!queryDosDevice(driveLetter, &deviceName)) {
        return "";
    }

    parts.prepend(deviceName);
    return parts.join("\\");
}

std::vector<uint8_t> serializeConfiguration(const QStringList &appPaths) {
    size_t totalStringSize = 0;

    QStringList dosPaths{};
    for (const auto &path: appPaths) {
        auto dosPath = toDevicePath(path);
        if (dosPath.isEmpty())
            continue;

        dosPaths.append(dosPath);
        totalStringSize += dosPath.toStdWString().size() * sizeof(wchar_t);
    }

    size_t totalBufferSize =
            sizeof(CONFIGURATION_HEADER) + sizeof(CONFIGURATION_ENTRY) * dosPaths.size() + totalStringSize;

    std::vector<uint8_t> buffer{};
    buffer.resize(totalBufferSize, 0);

    auto header = reinterpret_cast<CONFIGURATION_HEADER *>(buffer.data());
    auto entry = reinterpret_cast<CONFIGURATION_ENTRY *>(header + 1);
    auto stringBuffer = reinterpret_cast<uint8_t *>(entry + dosPaths.size());

    size_t currentStringOffset = 0;

    for (const auto &path: dosPaths) {
        auto pathString = path.toStdWString();
        size_t stringLength = pathString.size() * sizeof(wchar_t);

        entry->ImageNameLength = (USHORT) stringLength;
        entry->ImageNameOffset = currentStringOffset;

        memcpy(stringBuffer + currentStringOffset, pathString.data(), stringLength);
        currentStringOffset += stringLength;

        ++entry;
    }

    header->NumEntries = dosPaths.size();
    header->TotalLength = totalBufferSize;

    return buffer;
}

void getAddress(int adapterIndex, IN_ADDR *outIpv4, IN6_ADDR *outIpv6) {
    QNetworkInterface target = QNetworkInterface::interfaceFromIndex(adapterIndex);

    for (auto address: target.addressEntries()) {
        if (address.ip().protocol() == QAbstractSocket::IPv4Protocol) {
            std::wstring addr = address.ip().toString().toStdWString();
            bool ok = InetPtonW(AF_INET, addr.c_str(), outIpv4);
            if (ok != 1) {
                logger.debug() << "Ipv4 conversion error " << WSAGetLastError();
            } else {
                break;
            }
        }
    }

    for (auto address: target.addressEntries()) {
        if (address.ip().protocol() == QAbstractSocket::IPv6Protocol) {
            QString addr = address.ip().toString();
            std::wstring addrWide = addr.split("%").takeFirst().toStdWString();

            bool ok = InetPtonW(AF_INET6, addrWide.c_str(), outIpv6);
            if (ok != 1) {
                logger.debug() << "Ipv6 converesion error " << WSAGetLastError();
            } else {
                break;
            }
        }
    }
}
