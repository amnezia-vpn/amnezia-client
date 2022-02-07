#include "wireguardtunnelservice.h"
#include <strsafe.h>
#include <Windows.h>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 3) {
        debug_log(L"Wrong argument provided");
        return 1;
    }
    TCHAR option[20];
    TCHAR configFile[5000];

    StringCchCopy(option, 20, argv[1]);
    StringCchCopy(configFile, 5000, argv[2]);

    WireguardTunnelService tunnel(configFile);

    if (lstrcmpi(option, TEXT("--run")) == 0) {
        debug_log(L"start tunnel");
        tunnel.startTunnel();
    } else if (lstrcmpi(option, TEXT("--add")) == 0) {
        tunnel.addService();
    } else if (lstrcmpi(option, TEXT("--remove")) == 0) {
        tunnel.removeService();
    } else {
        debug_log(L"Wrong argument provided");
        return 1;
    }
    return 0;
}
