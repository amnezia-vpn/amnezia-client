#pragma once

class WinSplitTunnelService {
public:
    enum class InstallError {
        None,
        AlreadyInstalled,
        DriverNotFound,
        ServiceManager,
        ServiceOpen,
        ServiceCreate,
        ServiceStart
    };

    static bool KillConflictingServices();
    static InstallError InstallService(bool allowDelete = true);
};
