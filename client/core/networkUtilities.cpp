#include "networkUtilities.h"
#include <QtNetwork/qnetworkinterface.h>
#include <cstddef>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <Ipexport.h>
    #include <Ws2tcpip.h>
    #include <ws2ipdef.h>
    #include <Iphlpapi.h>
    #include <Iptypes.h>
    #include <WinSock2.h>
    #include <winsock.h>
    #include <QNetworkInterface>
    #include "qendian.h"
    #include <QSettings>
#endif
#ifdef Q_OS_LINUX
    #include <arpa/inet.h>
    #include <linux/netlink.h>
    #include <linux/rtnetlink.h>
    #include <net/if.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    #include <sys/param.h>
    #include <sys/sysctl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <net/route.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <net/if_dl.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <ifaddrs.h>
    #include <net/if.h>
#endif

#include <QHostAddress>
#include <QHostInfo>

QRegularExpression NetworkUtilities::ipAddressRegExp()
{
    return QRegularExpression("^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])(\\.(?!$)|$)){4}$");
}

QRegularExpression NetworkUtilities::ipAddressPortRegExp()
{
    return QRegularExpression("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
                              "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])(\\:[0-9]{1,5}){0,1}$");
}

QRegExp NetworkUtilities::ipAddressWithSubnetRegExp()
{
    return QRegExp("(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
                   "(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])(\\/[0-9]{1,2}){0,1}");
}

QRegExp NetworkUtilities::ipNetwork24RegExp()
{
    return QRegExp("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}"
                   "0$");
}

QRegExp NetworkUtilities::ipPortRegExp()
{
    return QRegExp("^()([1-9]|[1-5]?[0-9]{2,4}|6[1-4][0-9]{3}|65[1-4][0-9]{2}|655[1-2][0-9]|6553[1-5])$");
}

QRegExp NetworkUtilities::domainRegExp()
{
    return QRegExp("(((?!\\-))(xn\\-\\-)?[a-z0-9\\-_]{0,61}[a-z0-9]{1,1}\\.)*(xn\\-\\-)?([a-z0-9\\-]{1,61}|[a-z0-"
                   "9\\-]{1,30})\\.[a-z]{2,}");
}

QString NetworkUtilities::netMaskFromIpWithSubnet(const QString ip)
{
    if (!ip.contains("/"))
        return "255.255.255.255";

    bool ok;
    int prefix = ip.split("/").at(1).toInt(&ok);
    if (!ok)
        return "255.255.255.255";

    unsigned long mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;

    return QString("%1.%2.%3.%4").arg(mask >> 24).arg((mask >> 16) & 0xFF).arg((mask >> 8) & 0xFF).arg(mask & 0xFF);
}

QString NetworkUtilities::ipAddressFromIpWithSubnet(const QString ip)
{
    if (ip.count(".") != 3)
        return "";
    return ip.split("/").first();
}

QStringList NetworkUtilities::summarizeRoutes(const QStringList &ips, const QString cidr)
{
    //    QMap<int, int>
    //    QHostAddress

           //    QMap<QString, QStringList> subnets; // <"a.b", <list subnets>>

           //    for (const QString &ip : ips) {
           //        if (ip.count(".") != 3) continue;

           //        const QStringList &parts = ip.split(".");
           //        subnets[parts.at(0) + "." + parts.at(1)].append(ip);
           //    }

    return QStringList();
}

QString NetworkUtilities::getIPAddress(const QString &host)
{
    QHostAddress address(host);
    if (QAbstractSocket::IPv4Protocol == address.protocol()) {
        return host;
    } else if (QAbstractSocket::IPv6Protocol == address.protocol()) {
        return host;
    }

    QList<QHostAddress> addresses = QHostInfo::fromName(host).addresses();
    if (!addresses.isEmpty()) {
        return addresses.first().toString();
    }
    qDebug() << "Unable to resolve address for " << host;
    return "";
}

QString NetworkUtilities::getStringBetween(const QString &s, const QString &a, const QString &b)
{
    int ap = s.indexOf(a), bp = s.indexOf(b, ap + a.length());
    if (ap < 0 || bp < 0)
        return QString();
    ap += a.length();
    if (bp - ap <= 0)
        return QString();
    return s.mid(ap, bp - ap).trimmed();
}

bool NetworkUtilities::checkIPv4Format(const QString &ip)
{
    if (ip.isEmpty())
        return false;
    int count = ip.count(".");
    if (count != 3)
        return false;

    QHostAddress addr(ip);
    return (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol);
}

bool NetworkUtilities::checkIpSubnetFormat(const QString &ip)
{
    if (!ip.contains("/"))
        return checkIPv4Format(ip);

    QStringList parts = ip.split("/");
    if (parts.size() != 2)
        return false;

    bool ok;
    int subnet = parts.at(1).toInt(&ok);
    if (subnet >= 0 && subnet <= 32 && ok)
        return checkIPv4Format(parts.at(0));
    else
        return false;
}

// static
int NetworkUtilities::AdapterIndexTo(const QHostAddress& dst) {
#ifdef Q_OS_WIN
    qDebug() << "Getting Current Internet Adapter that routes to"
             << dst.toString();
    quint32 ipBigEndian;
    quint32 ip = dst.toIPv4Address();
    qToBigEndian(ip, &ipBigEndian);
    _MIB_IPFORWARDROW routeInfo;
    auto result = GetBestRoute(ipBigEndian, 0, &routeInfo);
    if (result != NO_ERROR) {
        return -1;
    }
    auto adapter =
        QNetworkInterface::interfaceFromIndex(routeInfo.dwForwardIfIndex);
    qDebug() << "Internet Adapter:" << adapter.name();
    return routeInfo.dwForwardIfIndex;
#endif
    return 0;
}

bool NetworkUtilities::checkIpv6Enabled() {
#ifdef Q_OS_WIN
    QSettings RegHLM("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters",
                     QSettings::NativeFormat);
    int ret = RegHLM.value("DisabledComponents", 0).toInt();
    qDebug() << "Check for Windows disabled IPv6 return " << ret;
    return (ret != 255);
#endif
    return true;
}

#ifdef Q_OS_WIN
DWORD GetAdaptersAddressesWrapper(const ULONG Family,
                                  const ULONG Flags,
                                  const PVOID Reserved,
                                  _Out_ PIP_ADAPTER_ADDRESSES& pAdapterAddresses) {
    DWORD dwRetVal = 0;
    int iter = 0;
    constexpr int max_iter = 3;
    ULONG AdapterAddressesLen = 15000;
    do {
        // xassert2(pAdapterAddresses == nullptr);
        pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(AdapterAddressesLen);
        if (pAdapterAddresses == nullptr) {
            qDebug() << "can not malloc" << AdapterAddressesLen << "bytes";
            return ERROR_OUTOFMEMORY;
        }

        dwRetVal = GetAdaptersAddresses(Family, Flags, NULL, pAdapterAddresses, &AdapterAddressesLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterAddresses);
            pAdapterAddresses = nullptr;
        } else {
            break;
        }

        iter++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (iter < max_iter));

    if (dwRetVal != NO_ERROR) {
        qDebug() << "Family: " << Family << ", Flags: " << Flags << " AdapterAddressesLen: " << AdapterAddressesLen <<
                ", dwRetVal:" << dwRetVal << ", iter: " << iter;
        if (pAdapterAddresses) {
            free(pAdapterAddresses);
            pAdapterAddresses = nullptr;
        }
    }

    return dwRetVal;
}
#endif

QPair<QString, QNetworkInterface> NetworkUtilities::getGatewayAndIface()
{
#ifdef Q_OS_WIN
    constexpr int BUFF_LEN = 100;
    char buff[BUFF_LEN] = {'\0'};

    QString resGateway;
    int resIndex = -1;

    PIP_ADAPTER_ADDRESSES pAdapterAddresses = nullptr;
    DWORD dwRetVal =
            GetAdaptersAddressesWrapper(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAdapterAddresses);

    if (dwRetVal != NO_ERROR) {
        qDebug() << "ipv4 stack detect GetAdaptersAddresses failed.";
        return {};
    }

    PIP_ADAPTER_ADDRESSES pCurAddress = pAdapterAddresses;
    while (pCurAddress) {
        PIP_ADAPTER_GATEWAY_ADDRESS_LH gateway = pCurAddress->FirstGatewayAddress;
        if (gateway) {
            SOCKET_ADDRESS gateway_address = gateway->Address;
            if (gateway->Address.lpSockaddr->sa_family == AF_INET) {
                sockaddr_in* sa_in = (sockaddr_in*)gateway->Address.lpSockaddr;
                QString gw = inet_ntop(AF_INET, &(sa_in->sin_addr), buff, BUFF_LEN);
                qDebug() <<  "gateway IPV4:" << gw;
                struct sockaddr_in addr;
                if (inet_pton(AF_INET, buff, &addr.sin_addr) == 1) {
                    qDebug() <<  "this is true v4 !";
                    
                    resGateway = gw;
                    resIndex = pCurAddress->IfIndex;
                }
            }
        }
        pCurAddress = pCurAddress->Next;
    }

    free(pAdapterAddresses);
    return { resGateway, QNetworkInterface::interfaceFromIndex(resIndex) };
#endif
#ifdef Q_OS_LINUX
    static constexpr size_t BUFFER_SIZE = 8192;

    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        perror("socket failed");
        return {};
    }

    struct timeval tv { 1, 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct {
        struct nlmsghdr hdr;
        struct rtmsg    rt;
    } req {};
    req.hdr.nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.hdr.nlmsg_type  = RTM_GETROUTE;
    req.hdr.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.hdr.nlmsg_seq   = 1;
    req.hdr.nlmsg_pid   = static_cast<uint32_t>(getpid());
    req.rt.rtm_family   = AF_INET;

    if (send(sock, &req, req.hdr.nlmsg_len, 0) < 0) {
        perror("send failed");
        close(sock);
        return {};
    }

    char   buffer[BUFFER_SIZE];
    size_t total_len = 0;
    bool   done = false;

    while (!done && total_len < BUFFER_SIZE) {
        ssize_t n = recv(sock, buffer + total_len, BUFFER_SIZE - total_len, 0);
        if (n <= 0)
            break;

        int scan_len = static_cast<int>(n);
        for (struct nlmsghdr *h = reinterpret_cast<struct nlmsghdr *>(buffer + total_len);
             NLMSG_OK(h, scan_len);
             h = NLMSG_NEXT(h, scan_len))
        {
            if (h->nlmsg_type == NLMSG_DONE || h->nlmsg_type == NLMSG_ERROR) {
                done = true;
                break;
            }
        }
        total_len += static_cast<size_t>(n);
    }

    QString resultGw;
    QString resultIf;
    int remaining = static_cast<int>(total_len);

    for (struct nlmsghdr *nlh = reinterpret_cast<struct nlmsghdr *>(buffer);
         NLMSG_OK(nlh, remaining);
         nlh = NLMSG_NEXT(nlh, remaining))
    {
        if (nlh->nlmsg_type == NLMSG_DONE || nlh->nlmsg_type == NLMSG_ERROR)
            break;
        if (nlh->nlmsg_type != RTM_NEWROUTE)
            continue;

        struct rtmsg *rt = static_cast<struct rtmsg *>(NLMSG_DATA(nlh));
        if (rt->rtm_table != RT_TABLE_MAIN || rt->rtm_family != AF_INET)
            continue;

        char route_gw[INET_ADDRSTRLEN] = {};
        char route_if[IF_NAMESIZE]     = {};
        int  attr_len = RTM_PAYLOAD(nlh);

        for (struct rtattr *rta = RTM_RTA(rt);
             RTA_OK(rta, attr_len);
             rta = RTA_NEXT(rta, attr_len))
        {
            if (rta->rta_type == RTA_GATEWAY)
                inet_ntop(AF_INET, RTA_DATA(rta), route_gw, sizeof(route_gw));
            else if (rta->rta_type == RTA_OIF)
                if_indextoname(*static_cast<int *>(RTA_DATA(rta)), route_if);
        }

        if (!route_gw[0] || !route_if[0])
            continue;

        QString ifStr(route_if);
        if (ifStr.startsWith(QLatin1String("amn")) ||
            ifStr.startsWith(QLatin1String("tun")))
            continue;

        resultGw = QString::fromLatin1(route_gw);
        resultIf = QString::fromLatin1(route_if);
        qDebug() << "Gateway " << route_gw << " for interface " << route_if;
        break;
    }

    close(sock);
    return { resultGw, QNetworkInterface::interfaceFromName(resultIf) };
#endif
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    QString gateway;
    int index = -1;

    int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_FLAGS, RTF_GATEWAY};
    int afinet_type[] = {AF_INET, AF_INET6};

    for (int ip_type = 0; ip_type <= 1; ip_type++)
    {
        mib[3] = afinet_type[ip_type];

        size_t needed = 0;
        if (sysctl(mib, sizeof(mib) / sizeof(int), nullptr, &needed, nullptr, 0) < 0)
            return {};

        char* buf;
        if ((buf = new char[needed]) == 0)
            return {};

        if (sysctl(mib, sizeof(mib) / sizeof(int), buf, &needed, nullptr, 0) < 0)
        {
            qDebug() << "sysctl: net.route.0.0.dump";
            delete[] buf;
            return {};
        }

        struct rt_msghdr* rt;
        for (char* p = buf; p < buf + needed; p += rt->rtm_msglen)
        {
            rt = reinterpret_cast<struct rt_msghdr*>(p);
            struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(rt + 1);
            struct sockaddr* sa_tab[RTAX_MAX];
            for (int i = 0; i < RTAX_MAX; i++)
            {
                if (rt->rtm_addrs & (1 << i))
                {
                    sa_tab[i] = sa;
                    sa = reinterpret_cast<struct sockaddr*>(
                            reinterpret_cast<char*>(sa) +
                            ((sa->sa_len) > 0 ? (1 + (((sa->sa_len) - 1) | (sizeof(long) - 1))) : sizeof(long)));
                }
                else
                {
                    sa_tab[i] = nullptr;
                }
            }

            if (((rt->rtm_addrs & (RTA_DST | RTA_GATEWAY)) == (RTA_DST | RTA_GATEWAY)) &&
                sa_tab[RTAX_DST]->sa_family == afinet_type[ip_type] &&
                sa_tab[RTAX_GATEWAY]->sa_family == afinet_type[ip_type])
            {
                if (afinet_type[ip_type] == AF_INET)
                {
                    if ((reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_DST]))->sin_addr.s_addr == 0)
                    {
                        char dstStr4[INET_ADDRSTRLEN];
                        char srcStr4[INET_ADDRSTRLEN];
                        memcpy(srcStr4,
                               &(reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_GATEWAY]))->sin_addr,
                               sizeof(struct in_addr));
                        if (inet_ntop(AF_INET, srcStr4, dstStr4, INET_ADDRSTRLEN) != nullptr)
                        {
                            gateway = dstStr4;
                            index = rt->rtm_index;
                        }
                        break;
                    }
                }
                else if (afinet_type[ip_type] == AF_INET6)
                {
                    if ((reinterpret_cast<struct sockaddr_in*>(sa_tab[RTAX_DST]))->sin_addr.s_addr == 0)
                    {
                        char dstStr6[INET6_ADDRSTRLEN];
                        char srcStr6[INET6_ADDRSTRLEN];
                        memcpy(srcStr6,
                               &(reinterpret_cast<struct sockaddr_in6*>(sa_tab[RTAX_GATEWAY]))->sin6_addr,
                               sizeof(struct in6_addr));
                        if (inet_ntop(AF_INET6, srcStr6, dstStr6, INET6_ADDRSTRLEN) != nullptr)
                        {
                            gateway = dstStr6;
                            index = rt->rtm_index;
                        }
                        break;
                    }
                }
            }
        }
        free(buf);
    }

    return { gateway, QNetworkInterface::interfaceFromIndex(index) };
#endif
}
