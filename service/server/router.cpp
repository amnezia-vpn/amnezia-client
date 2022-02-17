#include "router.h"

#ifdef Q_OS_WIN
#include "router_win.h"
#elif defined(Q_OS_MAC)
#include "router_mac.h"
#elif defined Q_OS_LINUX
#include "router_linux.h"
#endif

#ifdef Q_OS_WIN
#include <QNetworkInterface>
#include "windowsfirewall.h"
#include <cstring>

namespace
{
// TODO:FIXME try to get this constexpr variable from CORE code
constexpr char IKEV2[]{"AmneziaVPN IKEv2"};
constexpr char WG[]{"AmneziaVPN.WireGuard0"};
constexpr char OVPN[]{"TAP-Windows Adapter V9"};

bool is_eth_adapter_activated(char* ethName)
{
    auto convert_wide_to_ansi = [](const std::wstring& widestring)->std::string{
        auto nchars = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    widestring.c_str(),
                    static_cast<int>(widestring.length() + 1),
                    nullptr,
                    0,
                    nullptr,
                    nullptr);
        std::string converted_string{};
        converted_string.resize(nchars);
        WideCharToMultiByte(CP_ACP,
                            0,
                            widestring.c_str(),
                            -1,
                            &converted_string[0],
                static_cast<int>(widestring.length()),
                nullptr,
                nullptr);
        return converted_string;
    };

    constexpr ULONG MAX_BUFFER_SIZE = 15000;
    ULONG bufferSize = MAX_BUFFER_SIZE;
    IP_ADAPTER_ADDRESSES *pAdapterAddresses = (IP_ADAPTER_ADDRESSES *)HeapAlloc(GetProcessHeap(), 0, bufferSize);
    ::GetAdaptersAddresses(AF_INET, 0, nullptr, pAdapterAddresses, &bufferSize);
    do
    {
        const auto Descr = convert_wide_to_ansi(pAdapterAddresses->Description);
        if (strcmp(ethName, Descr.data()) == 0) //the same
        {
            if (pAdapterAddresses->OperStatus == IfOperStatusUp){
                return true;
            }
        }
        if (pAdapterAddresses->Next != 0)
        {
            pAdapterAddresses = pAdapterAddresses->Next;
        }
    } while (pAdapterAddresses->Next != 0);
    //::HeapFree(GetProcessHeap(), 0, pAdapterAddresses);
    return false;
}

bool get_eth_name(const QString &adp_name)
{
    IP_ADAPTER_INFO *pAdapterInfo{nullptr};
    pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    ULONG buflen = sizeof(IP_ADAPTER_INFO);
    if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)
                malloc(buflen);
    }
    if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR)
    {
        IP_ADAPTER_INFO *pAdapter{pAdapterInfo};
        if (pAdapter == nullptr)
            return false;
        while (pAdapter != nullptr)
        {
            const auto adp_acitvated = is_eth_adapter_activated(pAdapter->Description);
            if (adp_name == pAdapter->Description && adp_acitvated){
                qDebug() << "We will work with:";
                qDebug() << "Adapter Name: " << pAdapter->AdapterName;
                qDebug() << "Adapter Desc: " << pAdapter->Description;
                qDebug() << "Adapter Index: " << pAdapter->Index;
                qDebug() << "Adapter activated:"<<adp_acitvated;
                free(pAdapterInfo);
                return true;
            }
            pAdapter = pAdapter->Next;
        }
    }
    free(pAdapterInfo);
    return false;
}

void enable_killswitch()
{
    auto VPN_LIST = []() -> int
    {
        const auto adapterList = QNetworkInterface::allInterfaces();
        //qAsConst
        for (const auto &adapter : adapterList)
        {
            bool finded{false};
            finded = get_eth_name(OVPN);
            if (adapter.humanReadableName().contains(IKEV2) ||
                    adapter.humanReadableName().contains(WG) ||
                    finded ||
                    adapter.humanReadableName().contains(OVPN))
            {
                qDebug() << "Network adapter for 'kill switch' option finded: " << adapter.humanReadableName();
                return adapter.index();
            }
        }
        return -1;
    };
    const auto &current_vpn = VPN_LIST();
    if (current_vpn != -1)
    {
        qInfo() << "KillSwitch option activated";
        auto cf = WindowsFirewall::instance();
        cf->enableKillSwitch(current_vpn);
    }
    else
    {
        // TODO::FIXME
        qCritical().noquote() << "No any adapters was found, it's error";
    }
}
} // end namespace
#endif

// TODO::FIXME when wireguard is activated, the adapter will alaviable
// after a couple of seconds
#include <thread>

int Router::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    auto value = RouterWin::Instance().routeAddList(gw, ips);
    // TODO::FIXME the next sleep is need only for wireguard
    std::this_thread::sleep_for(std::chrono::seconds(5));
    enable_killswitch();
    return value;
    // return RouterWin::Instance().routeAddList(gw, ips);
#elif defined(Q_OS_MAC)
    return RouterMac::Instance().routeAddList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeAddList(gw, ips);
#endif
}

bool Router::clearSavedRoutes()
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().clearSavedRoutes();
#elif defined(Q_OS_MAC)
    return RouterMac::Instance().clearSavedRoutes();
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().clearSavedRoutes();
#endif
}

int Router::routeDeleteList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeDeleteList(gw, ips);
#elif defined(Q_OS_MAC)
    return RouterMac::Instance().routeDeleteList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeDeleteList(gw, ips);
#endif
}

void Router::flushDns()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().flushDns();
#elif defined(Q_OS_MAC)
    RouterMac::Instance().flushDns();
#elif defined Q_OS_LINUX
    RouterLinux::Instance().flushDns();
#endif
}

void Router::resetIpStack()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().resetIpStack();
#elif defined(Q_OS_MAC)
    // todo fixme
#elif defined Q_OS_LINUX
    // todo fixme
#endif
}
