#include "router.h"

#ifdef Q_OS_WIN
#include "router_win.h"
#elif defined (Q_OS_MAC)
#include "router_mac.h"
#elif defined Q_OS_LINUX
#include "router_linux.h"
#endif

#ifdef Q_OS_WIN
#include <QNetworkInterface>
#include "windowsfirewall.h"
//#include "netadpinfo.h"
#include <cstring>

namespace {

constexpr char IKEV2[] {"AmneziaVPN IKEv2"};
constexpr char WG[] {"AmneziaVPN.WireGuard0"};
constexpr char OVPN[] {"TAP-Windows Adapter V9"};

bool get_eth_name(const QString &adp_name){
    //char buff[128]{};
    PIP_ADAPTER_INFO pAdapterInfo{};
    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
    ULONG buflen = sizeof(IP_ADAPTER_INFO);
    if(GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)
                malloc(buflen);
    }
    if(GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            qDebug()<<"Adapter Name: "<< pAdapter->AdapterName;
            qDebug()<<"Adapter Desc: "<<pAdapter->Description;
            if (adp_name == pAdapter->Description)
                return true;
            pAdapter = pAdapter->Next;
        }
    }
    return false;
}

void enable_killswitch(){
    auto VPN_LIST = []()->int{
        auto adapterList = QNetworkInterface::allInterfaces();
        for (const auto& adapter : adapterList) {
            bool finded{false};
            finded = get_eth_name(OVPN);
            if (    adapter.humanReadableName().contains(IKEV2) ||
                    adapter.humanReadableName().contains(WG) ||
                    finded ||
                    adapter.humanReadableName().contains(OVPN)
                    ) {
                return adapter.index();
            }
        }
        return -1;
    };
    const auto &current_vpn = VPN_LIST();
    if (current_vpn != -1){
        qInfo()<<"KillSwitch activated";
        auto cf = WindowsFirewall::instance();
        //cf->enableKillSwitch(current_vpn);
        cf->disableKillSwitch();
    }else{
        qCritical().noquote() <<"No any adapters was found";
    }
}
}// end namespace
#endif


int Router::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    auto value = RouterWin::Instance().routeAddList(gw, ips);
    enable_killswitch();
    return value;
    //return RouterWin::Instance().routeAddList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeAddList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeAddList(gw, ips);
#endif
}

bool Router::clearSavedRoutes()
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().clearSavedRoutes();
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().clearSavedRoutes();
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().clearSavedRoutes();
#endif
}

int Router::routeDeleteList(const QString &gw, const QStringList &ips)
{
#ifdef Q_OS_WIN
    return RouterWin::Instance().routeDeleteList(gw, ips);
#elif defined (Q_OS_MAC)
    return RouterMac::Instance().routeDeleteList(gw, ips);
#elif defined Q_OS_LINUX
    return RouterLinux::Instance().routeDeleteList(gw, ips);
#endif
}

void Router::flushDns()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().flushDns();
#elif defined (Q_OS_MAC)
    RouterMac::Instance().flushDns();
#elif defined Q_OS_LINUX
    RouterLinux::Instance().flushDns();
#endif
}

void Router::resetIpStack()
{
#ifdef Q_OS_WIN
    RouterWin::Instance().resetIpStack();
#elif defined (Q_OS_MAC)
    // todo fixme
#elif defined Q_OS_LINUX
    // todo fixme
#endif
}

