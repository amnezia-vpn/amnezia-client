#include "networkUtilities.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <Ipexport.h>
    #include <Ws2tcpip.h>
    #include <ws2ipdef.h>
    #include <stdint.h>
    #include <Iphlpapi.h>
    #include <Iptypes.h>
    #include <WinSock2.h>
    #include <winsock.h>
    #include <QNetworkInterface>
    #include "qendian.h"
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
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS)
    #include <sys/param.h>
    #include <sys/sysctl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <net/route.h>
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
    quint32_be ipBigEndian;
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

QString NetworkUtilities::getGatewayAndIface()
{
#ifdef Q_OS_WIN
    constexpr int BUFF_LEN = 100;
    char buff[BUFF_LEN] = {'\0'};
    QString result;

    PIP_ADAPTER_ADDRESSES pAdapterAddresses = nullptr;
    DWORD dwRetVal =
            GetAdaptersAddressesWrapper(AF_INET, GAA_FLAG_INCLUDE_GATEWAYS, NULL, pAdapterAddresses);

    if (dwRetVal != NO_ERROR) {
        qDebug() << "ipv4 stack detect GetAdaptersAddresses failed.";
        return "";
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
                    result = gw;
                }
            }
        }
        pCurAddress = pCurAddress->Next;
    }

    free(pAdapterAddresses);
    return result;
#endif
#ifdef Q_OS_LINUX
    constexpr int BUFFER_SIZE = 100;
    int     received_bytes = 0, msg_len = 0, route_attribute_len = 0;
    int     sock = -1, msgseq = 0;
    struct  nlmsghdr *nlh, *nlmsg;
    struct  rtmsg *route_entry;
    // This struct contain route attributes (route type)
    struct  rtattr *route_attribute;
    char    gateway_address[INET_ADDRSTRLEN], interface[IF_NAMESIZE];
    char    msgbuf[BUFFER_SIZE], buffer[BUFFER_SIZE];
    char    *ptr = buffer;
    struct timeval tv;

    if ((sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
        perror("socket failed");
        return "";
    }

    memset(msgbuf, 0, sizeof(msgbuf));
    memset(gateway_address, 0, sizeof(gateway_address));
    memset(interface, 0, sizeof(interface));
    memset(buffer, 0, sizeof(buffer));

    /* point the header and the msg structure pointers into the buffer */
    nlmsg = (struct nlmsghdr *)msgbuf;

    /* Fill in the nlmsg header*/
    nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlmsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
    nlmsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
    nlmsg->nlmsg_seq = msgseq++; // Sequence of the message packet.
    nlmsg->nlmsg_pid = getpid(); // PID of process sending the request.

    /* 1 Sec Timeout to avoid stall */
    tv.tv_sec = 1;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    /* send msg */
    if (send(sock, nlmsg, nlmsg->nlmsg_len, 0) < 0) {
        perror("send failed");
        return "";
    }

    /* receive response */
    do
    {
        received_bytes = recv(sock, ptr, sizeof(buffer) - msg_len, 0);
        if (received_bytes < 0) {
            perror("Error in recv");
            return "";
        }

        nlh = (struct nlmsghdr *) ptr;

        /* Check if the header is valid */
        if((NLMSG_OK(nlmsg, received_bytes) == 0) ||
            (nlmsg->nlmsg_type == NLMSG_ERROR))
        {
            perror("Error in received packet");
            return "";
        }

        /* If we received all data break */
        if (nlh->nlmsg_type == NLMSG_DONE)
            break;
        else {
            ptr += received_bytes;
            msg_len += received_bytes;
        }

        /* Break if its not a multi part message */
        if ((nlmsg->nlmsg_flags & NLM_F_MULTI) == 0)
            break;
    }
    while ((nlmsg->nlmsg_seq != msgseq) || (nlmsg->nlmsg_pid != getpid()));

    /* parse response */
    for ( ; NLMSG_OK(nlh, received_bytes); nlh = NLMSG_NEXT(nlh, received_bytes))
    {
        /* Get the route data */
        route_entry = (struct rtmsg *) NLMSG_DATA(nlh);

        /* We are just interested in main routing table */
        if (route_entry->rtm_table != RT_TABLE_MAIN)
            continue;

        route_attribute = (struct rtattr *) RTM_RTA(route_entry);
        route_attribute_len = RTM_PAYLOAD(nlh);

        /* Loop through all attributes */
        for ( ; RTA_OK(route_attribute, route_attribute_len);
             route_attribute = RTA_NEXT(route_attribute, route_attribute_len))
        {
            switch(route_attribute->rta_type) {
            case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(route_attribute), interface);
                break;
            case RTA_GATEWAY:
                inet_ntop(AF_INET, RTA_DATA(route_attribute),
                          gateway_address, sizeof(gateway_address));
                break;
            default:
                break;
            }
        }

        if ((*gateway_address) && (*interface)) {
            qDebug() << "Gateway " << gateway_address << " for interface " << interface;
            break;
        }
    }
    close(sock);
    return gateway_address;
#endif
#if defined(Q_OS_MAC) && !defined(Q_OS_IOS)
    QString gateway;
    int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_FLAGS, RTF_GATEWAY};
    int afinet_type[] = {AF_INET, AF_INET6};

    for (int ip_type = 0; ip_type <= 1; ip_type++)
    {
        mib[3] = afinet_type[ip_type];

        size_t needed = 0;
        if (sysctl(mib, sizeof(mib) / sizeof(int), nullptr, &needed, nullptr, 0) < 0)
            return "";

        char* buf;
        if ((buf = new char[needed]) == 0)
            return "";

        if (sysctl(mib, sizeof(mib) / sizeof(int), buf, &needed, nullptr, 0) < 0)
        {
            qDebug() << "sysctl: net.route.0.0.dump";
            delete[] buf;
            return gateway;
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
                            gateway = dstStr4;
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
                            gateway = dstStr6;
                        break;
                    }
                }
            }
        }
        free(buf);
    }

    return gateway;
#endif
}
