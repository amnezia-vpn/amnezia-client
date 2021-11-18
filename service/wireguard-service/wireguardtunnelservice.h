#ifndef WIREGUARDTUNNELSERVICE_H
#define WIREGUARDTUNNELSERVICE_H

#include <Windows.h>
#include <string>

#define SVCNAME TEXT("AmneziaVPNWireGuardService")

class WireguardTunnelService
{
public:
    WireguardTunnelService(const std::wstring& configFile);
    void addService();
    void removeService();
    int startTunnel();
private:
    std::wstring m_configFile;
};

void debug_log(const std::wstring& msg);

#endif // WIREGUARDTUNNELSERVICE_H
