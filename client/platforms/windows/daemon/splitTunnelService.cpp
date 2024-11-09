#include "splitTunnelService.h"
#include "logger.h"
#include "winHandle.h"

#include <QApplication>
#include <QFileInfo>

#include <Windows.h>

constexpr static const LPCWSTR DISPLAY_NAME = L"ZloVPN Split Tunnel Service";
constexpr static const auto DRIVER_FILENAME = "mullvad-split-tunnel.sys";
constexpr static const auto SPLIT_TUNNEL_SERVICE = L"mullvad-split-tunnel";
constexpr static const auto DRIVER_SERVICE_NAME = L"ZloVPNSplitTunnel";

namespace {
    Logger logger("WinSplitTunnelService");
}

enum class StopServiceError { None, ServiceNotFound, FailedToOpen, FailedToStop };

StopServiceError StopService(ServiceHandle &serviceManager, const LPCWSTR serviceName) {
    ServiceHandle service = OpenService(serviceManager, serviceName, SERVICE_STOP);
    if (!service) {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_SERVICE_DOES_NOT_EXIST) {
            return StopServiceError::ServiceNotFound;
        }
        qDebug() << "Failed to open service for stopping: " << serviceName << " error: " << lastError;
        return StopServiceError::FailedToOpen;
    }

    SERVICE_STATUS status{};
    if (!ControlService(service, SERVICE_CONTROL_STOP, &status)) {
        DWORD lastError = GetLastError();
        qDebug() << "Failed to stop service: " << serviceName << " error: " << lastError;
        return StopServiceError::FailedToStop;
    }

    return StopServiceError::None;
}

bool WinSplitTunnelService::KillConflictingServices() {
    ServiceHandle serviceManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!serviceManager) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to open service manager, error: " << lastError;
        return false;
    }

    StopService(serviceManager, L"AmneziaVPN-service");
    StopService(serviceManager, L"MullvadVPN");
}

WinSplitTunnelService::InstallError WinSplitTunnelService::InstallService(bool allowDelete) {
    QFileInfo driver(qApp->applicationDirPath() + "/" + DRIVER_FILENAME);
    if (!driver.exists()) {
        logger.debug() << "Driver not found at path: " << driver.absoluteFilePath();
        return InstallError::DriverNotFound;
    }

    QString path = driver.absoluteFilePath();
    LPCWSTR binPath = reinterpret_cast<LPCWSTR>(path.utf16());

    ServiceHandle serviceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (!serviceManager) {
        DWORD lastError = GetLastError();
        logger.debug() << "Failed to open service manager, error: " << lastError;
        return InstallError::ServiceManager;
    }

    ServiceHandle service = OpenService(serviceManager, DRIVER_SERVICE_NAME, SC_MANAGER_ALL_ACCESS);
    if (!service) {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_SERVICE_DOES_NOT_EXIST) {
            logger.debug() << "Failed to open service, error: " << lastError;
            return InstallError::ServiceOpen;
        }

        service = CreateService(serviceManager, DRIVER_SERVICE_NAME, DISPLAY_NAME, SERVICE_ALL_ACCESS,
                                SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, binPath, nullptr, 0,
                                nullptr, nullptr, nullptr);
        if (!service) {
            lastError = GetLastError();
            logger.debug() << "Failed to create service, error: " << lastError;
            return InstallError::ServiceCreate;
        }
    }

    if (!StartService(service, 0, nullptr)) {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND && allowDelete) {
            DeleteService(service);
            return InstallService(false);
        }

        if (lastError == ERROR_ALREADY_EXISTS) {
            logger.warning() << "A conflicting instance of the driver is running, hijacking...";
            return InstallError::None;
        }

        if (lastError != ERROR_SERVICE_ALREADY_RUNNING) {
            logger.debug() << "Failed to start service, error: " << lastError;
            return InstallError::ServiceStart;
        }
    }

    return InstallError::None;
}
