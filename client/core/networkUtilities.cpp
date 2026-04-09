#include "networkUtilities.h"
#include <QtNetwork/qnetworkinterface.h>
#include <cstddef>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QJsonObject>

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
#include <QUdpSocket>
#include <QTcpSocket>
#include <QSslSocket>
#include <QEventLoop>
#include <QTimer>
#include <QtEndian>
#include <QDebug>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QThread>
#include <QElapsedTimer>

namespace
{
    constexpr quint16 DNS_PORT = 53;
    constexpr quint16 DNS_TYPE_A = 1;      // A record
    constexpr quint16 DNS_CLASS_IN = 1;    // Internet class
    
    // DNS Header structure (RFC 1035)
    struct DnsHeader {
        quint16 id;        // Transaction ID
        quint16 flags;     // Flags
        quint16 qdcount;   // Question count
        quint16 ancount;   // Answer count
        quint16 nscount;   // Authority count
        quint16 arcount;   // Additional count
    };
    
    // Helper function to encode hostname to QNAME format
    QByteArray encodeDnsName(const QString &hostname)
    {
        QByteArray result;
        QStringList parts = hostname.split('.');
        
        for (const QString &part : parts) {
            if (part.length() > 63) {
                return QByteArray(); // Invalid
            }
            result.append(static_cast<char>(part.length()));
            result.append(part.toUtf8());
        }
        result.append(static_cast<char>(0)); // Null terminator
        
        return result;
    }
    
    // Helper function to build DNS query packet
    QByteArray buildDnsQuery(const QString &hostname, quint16 transactionId)
    {
        QByteArray packet;
        
        // DNS Header
        DnsHeader header;
        header.id = qToBigEndian(transactionId);
        header.flags = qToBigEndian<quint16>(0x0100); // Standard query, recursion desired
        header.qdcount = qToBigEndian<quint16>(1);   // One question
        header.ancount = 0;
        header.nscount = 0;
        header.arcount = 0;
        
        packet.append(reinterpret_cast<const char*>(&header), sizeof(DnsHeader));
        
        // Question section
        QByteArray qname = encodeDnsName(hostname);
        if (qname.isEmpty()) {
            return QByteArray();
        }
        packet.append(qname);
        
        // QTYPE (A record)
        quint16 qtype = qToBigEndian<quint16>(DNS_TYPE_A);
        packet.append(reinterpret_cast<const char*>(&qtype), sizeof(quint16));
        
        // QCLASS (IN)
        quint16 qclass = qToBigEndian<quint16>(DNS_CLASS_IN);
        packet.append(reinterpret_cast<const char*>(&qclass), sizeof(quint16));
        
        return packet;
    }
    
    // Helper function to parse DNS response and extract IP address
    QString parseDnsResponse(const QByteArray &response, bool isTcp)
    {
        if (response.size() < static_cast<int>(sizeof(DnsHeader))) {
            return QString();
        }
        
        // Skip length prefix for TCP
        int offset = isTcp ? 2 : 0;
        if (response.size() < offset + static_cast<int>(sizeof(DnsHeader))) {
            return QString();
        }
        
        // Parse header
        DnsHeader header;
        memcpy(&header, response.constData() + offset, sizeof(DnsHeader));
        offset += sizeof(DnsHeader);
        
        quint16 flags = qFromBigEndian(header.flags);
        quint16 ancount = qFromBigEndian(header.ancount);
        
        // Check if response is valid (QR bit set, no error)
        if ((flags & 0x8000) == 0 || (flags & 0x000F) != 0) {
            return QString(); // Not a response or has error
        }
        
        if (ancount == 0) {
            return QString(); // No answers
        }
        
        // Skip question section
        // Find end of QNAME (null terminator)
        while (offset < response.size() && response.at(offset) != 0) {
            quint8 length = static_cast<quint8>(response.at(offset));
            if (length > 63) {
                return QString(); // Invalid
            }
            offset += length + 1;
        }
        if (offset >= response.size()) {
            return QString();
        }
        offset++; // Skip null terminator
        
        // Skip QTYPE and QCLASS (4 bytes)
        offset += 4;
        
        // Parse answer section
        for (int i = 0; i < ancount && offset < response.size(); i++) {
            // Skip NAME (can be pointer or label)
            if (offset >= response.size()) {
                break;
            }
            
            quint8 nameByte = static_cast<quint8>(response.at(offset));
            if ((nameByte & 0xC0) == 0xC0) {
                // Pointer (compressed name)
                offset += 2;
            } else {
                // Label sequence
                while (offset < response.size() && response.at(offset) != 0) {
                    quint8 length = static_cast<quint8>(response.at(offset));
                    if (length > 63) {
                        return QString();
                    }
                    offset += length + 1;
                }
                offset++; // Skip null terminator
            }
            
            // Read TYPE, CLASS, TTL
            if (offset + 10 > response.size()) {
                break;
            }
            
            quint16 type = qFromBigEndian<quint16>(*reinterpret_cast<const quint16*>(response.constData() + offset));
            offset += 2;
            offset += 2; // Skip CLASS
            offset += 4; // Skip TTL
            
            // Read RDLENGTH and RDATA
            quint16 rdlength = qFromBigEndian<quint16>(*reinterpret_cast<const quint16*>(response.constData() + offset));
            offset += 2;
            
            if (type == DNS_TYPE_A && rdlength == 4) {
                // A record with IPv4 address
                if (offset + 4 > response.size()) {
                    break;
                }
                
                QHostAddress ip;
                ip.setAddress(qFromBigEndian<quint32>(*reinterpret_cast<const quint32*>(response.constData() + offset)));
                return ip.toString();
            }
            
            offset += rdlength; // Skip RDATA
        }
        
        return QString();
    }
}

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
        return {};
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
        return {};
    }

    /* receive response */
    do
    {
        received_bytes = recv(sock, ptr, sizeof(buffer) - msg_len, 0);
        if (received_bytes < 0) {
            perror("Error in recv");
            return {};
        }

        nlh = (struct nlmsghdr *) ptr;

        /* Check if the header is valid */
        if((NLMSG_OK(nlmsg, received_bytes) == 0) ||
            (nlmsg->nlmsg_type == NLMSG_ERROR))
        {
            perror("Error in received packet");
            return {};
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
    return { gateway_address, QNetworkInterface::interfaceFromName(interface) };
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

QString NetworkUtilities::resolveDns(const QString &hostname, const QString &dnsServer, DnsTransport transport,
                                      quint16 port, int timeoutMsecs, const QString &dohEndpoint)
{
    switch (transport) {
        case DnsTransport::Udp:
            return resolveDnsOverUdp(hostname, dnsServer, port, timeoutMsecs);
        case DnsTransport::Tcp:
            return resolveDnsOverTcp(hostname, dnsServer, port, timeoutMsecs);
        case DnsTransport::Tls:
            return resolveDnsOverTls(hostname, dnsServer, port, timeoutMsecs);
        case DnsTransport::Https:
            return resolveDnsOverHttps(hostname, dnsServer, dohEndpoint, timeoutMsecs);
        case DnsTransport::Quic:
            return resolveDnsOverQuic(hostname, dnsServer, port, timeoutMsecs);
    }
    return QString();
}

QString NetworkUtilities::resolveDnsOverUdp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QUdpSocket socket;
    
    // Генерируем случайный transaction ID
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    
    // Формируем DNS запрос
    QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        return QString();
    }
    
    // Отправляем запрос
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }
    
    qint64 bytesWritten = socket.writeDatagram(query, dnsAddress, port);
    if (bytesWritten != query.size()) {
        return QString();
    }
    
    // Ждем ответ с таймаутом
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = socket.receiveDatagram();
            if (datagram.isValid()) {
                response = datagram.data();
                responseReceived = true;
                loop.quit();
            }
        }
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    
    if (!responseReceived || response.isEmpty()) {
        return QString();
    }
    
    // Парсим ответ
    return parseDnsResponse(response, false);
}

QString NetworkUtilities::resolveDnsOverTcp(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QTcpSocket socket;
    
    // Подключаемся к DNS серверу
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }
    
    socket.connectToHost(dnsAddress, port);
    
    if (!socket.waitForConnected(timeoutMsecs)) {
        return QString();
    }
    
    // Генерируем случайный transaction ID
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    
    // Формируем DNS запрос
    QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        socket.close();
        return QString();
    }
    
    // Для TCP добавляем 2-байтовое поле длины перед пакетом
    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tcpQuery;
    tcpQuery.append(reinterpret_cast<const char*>(&length), sizeof(quint16));
    tcpQuery.append(query);
    
    // Отправляем запрос
    qint64 bytesWritten = socket.write(tcpQuery);
    if (bytesWritten != tcpQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        socket.close();
        return QString();
    }
    
    // Ждем ответ с таймаутом
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::readyRead, [&]() {
        // Читаем длину ответа (первые 2 байта)
        if (socket.bytesAvailable() >= 2 && response.isEmpty()) {
            QByteArray lengthBytes = socket.read(2);
            if (lengthBytes.size() == 2) {
                quint16 responseLength = qFromBigEndian<quint16>(*reinterpret_cast<const quint16*>(lengthBytes.constData()));
                // Читаем весь ответ
                while (socket.bytesAvailable() < responseLength) {
                    if (!socket.waitForReadyRead(timeoutMsecs / 2)) {
                        break;
                    }
                }
                if (socket.bytesAvailable() >= responseLength) {
                    response = socket.read(responseLength);
                    responseReceived = true;
                    loop.quit();
                }
            }
        }
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    
    socket.close();
    
    if (!responseReceived || response.isEmpty()) {
        return QString();
    }
    
    // Парсим ответ (для TCP уже без префикса длины)
    return parseDnsResponse(response, true);
}

QString NetworkUtilities::resolveDnsOverTls(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QSslSocket socket;
    
    // Подключаемся к DNS серверу через TLS
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }
    
    // Настраиваем SSL (отключаем проверку сертификата для простоты, можно добавить проверку позже)
    socket.setPeerVerifyMode(QSslSocket::QueryPeer);
    
    socket.connectToHostEncrypted(dnsAddress.toString(), port);
    
    if (!socket.waitForConnected(timeoutMsecs)) {
        return QString();
    }
    
    // Ждем завершения TLS handshake
    if (!socket.waitForEncrypted(timeoutMsecs)) {
        socket.close();
        return QString();
    }
    
    // Генерируем случайный transaction ID
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    
    // Формируем DNS запрос
    QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        socket.close();
        return QString();
    }
    
    // Для TLS (как и для TCP) добавляем 2-байтовое поле длины перед пакетом
    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tlsQuery;
    tlsQuery.append(reinterpret_cast<const char*>(&length), sizeof(quint16));
    tlsQuery.append(query);
    
    // Отправляем запрос
    qint64 bytesWritten = socket.write(tlsQuery);
    if (bytesWritten != tlsQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        socket.close();
        return QString();
    }
    
    // Ждем ответ с таймаутом
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QSslSocket::readyRead, [&]() {
        // Читаем длину ответа (первые 2 байта)
        if (socket.bytesAvailable() >= 2 && response.isEmpty()) {
            QByteArray lengthBytes = socket.read(2);
            if (lengthBytes.size() == 2) {
                quint16 responseLength = qFromBigEndian<quint16>(*reinterpret_cast<const quint16*>(lengthBytes.constData()));
                // Читаем весь ответ
                while (socket.bytesAvailable() < responseLength) {
                    if (!socket.waitForReadyRead(timeoutMsecs / 2)) {
                        break;
                    }
                }
                if (socket.bytesAvailable() >= responseLength) {
                    response = socket.read(responseLength);
                    responseReceived = true;
                    loop.quit();
                }
            }
        }
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    
    socket.close();
    
    if (!responseReceived || response.isEmpty()) {
        return QString();
    }
    
    // Парсим ответ (для TLS тот же формат что и для TCP)
    return parseDnsResponse(response, true);
}

QString NetworkUtilities::resolveDnsOverHttps(const QString &hostname, const QString &dnsServer, const QString &endpoint, int timeoutMsecs)
{
    // DNS over HTTPS использует указанный endpoint
    QString dohUrl = QString("https://%1%2").arg(dnsServer, endpoint);
    
    // Формируем DNS запрос
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        return QString();
    }
    
    // Создаем HTTP запрос
    QNetworkRequest request;
    request.setUrl(QUrl(dohUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/dns-message");
    request.setRawHeader("Accept", "application/dns-message");
    request.setTransferTimeout(timeoutMsecs);
    
    // Отправляем POST запрос
    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.post(request, query);
    
    // Ждем ответ с таймаутом
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, [&]() {
        if (reply->error() == QNetworkReply::NoError) {
            response = reply->readAll();
            responseReceived = true;
        }
        loop.quit();
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    
    reply->deleteLater();
    
    if (!responseReceived || response.isEmpty()) {
        return QString();
    }
    
    // Парсим DNS ответ (DoH возвращает чистый DNS пакет без префикса)
    return parseDnsResponse(response, false);
}

QString NetworkUtilities::resolveDnsOverQuic(const QString &hostname, const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    // DNS over QUIC использует QUIC протокол (UDP-based с TLS)
    // QUIC требует специальной библиотеки (quiche, msquic и т.д.)
    // Qt не имеет встроенной поддержки QUIC
    // Для упрощения используем QUdpSocket с TLS поверх UDP
    // В реальной реализации нужна библиотека QUIC
    
    QUdpSocket socket;
    
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        return QString();
    }
    
    // Формируем DNS запрос
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray query = buildDnsQuery(hostname, transactionId);
    if (query.isEmpty()) {
        return QString();
    }
    
    // Для QUIC нужен специальный формат с QUIC заголовками
    // Упрощенная версия: отправляем DNS пакет через UDP
    // В реальной реализации нужно:
    // 1. Установить QUIC соединение (Initial packet, Handshake)
    // 2. Отправить DNS запрос в QUIC stream
    // 3. Получить ответ из QUIC stream
    
    // Временная реализация: используем UDP как fallback
    // TODO: Реализовать полноценный QUIC протокол с использованием библиотеки
    
    qint64 bytesWritten = socket.writeDatagram(query, dnsAddress, port);
    if (bytesWritten != query.size()) {
        return QString();
    }
    
    // Ждем ответ с таймаутом
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = socket.receiveDatagram();
            if (datagram.isValid()) {
                response = datagram.data();
                responseReceived = true;
                loop.quit();
            }
        }
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    
    if (!responseReceived || response.isEmpty()) {
        return QString();
    }
    
    // Парсим DNS ответ
    return parseDnsResponse(response, false);
}

// ============== DNS Tunneling ==============

namespace {
    // EDNS0 option codes (same as backend)
    constexpr quint16 EDNS0_PAYLOAD_OPTION_CODE = 65001;      // Payload in request
    constexpr quint16 EDNS0_CHUNK_REQUEST_CODE = 65002;       // Client requests chunk
    constexpr quint16 EDNS0_CHUNK_RESPONSE_CODE = 65003;      // Server responds with chunk meta
    
    // Chunk metadata from server response (24 bytes)
    struct ChunkMeta {
        QByteArray chunkId;     // 16 bytes
        quint16 totalChunks;    // big endian
        quint16 chunkIndex;     // big endian
        quint32 totalSize;      // big endian
    };
    
    // Helper to append big-endian uint16
    void appendUint16BE(QByteArray &data, quint16 value) {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }
    
    // Build DNS TXT query for requesting a specific chunk (no payload, just chunkId + index)
    QByteArray buildDnsChunkRequest(const QString &queryName, quint16 transactionId, 
                                     const QByteArray &chunkId, quint16 chunkIndex)
    {
        QByteArray query;
        
        // DNS Header (12 bytes)
        appendUint16BE(query, transactionId);
        appendUint16BE(query, 0x0100);         // Flags: standard query, RD=1
        appendUint16BE(query, 1);              // Questions: 1
        appendUint16BE(query, 0);              // Answers: 0
        appendUint16BE(query, 0);              // Authority: 0
        appendUint16BE(query, 1);              // Additional: 1 (EDNS0 OPT)
        
        // Question section - QNAME
        QStringList labels = queryName.split('.');
        for (const QString &label : labels) {
            QByteArray labelBytes = label.toUtf8();
            query.append(static_cast<char>(labelBytes.size()));
            query.append(labelBytes);
        }
        query.append(static_cast<char>(0));    // Root label
        appendUint16BE(query, 16);             // QTYPE: TXT
        appendUint16BE(query, 1);              // QCLASS: IN
        
        // Additional section - EDNS0 OPT record with chunk request
        // Format: chunkId(16) + chunkIndex(2) = 18 bytes
        quint16 optionDataLen = 4 + 18; // code(2) + length(2) + data(18)
        
        query.append(static_cast<char>(0));    // Name: root
        appendUint16BE(query, 41);             // TYPE: OPT
        appendUint16BE(query, 4096);           // CLASS: UDP payload size
        query.append(static_cast<char>(0));    // Extended RCODE
        query.append(static_cast<char>(0));    // EDNS version
        appendUint16BE(query, 0);              // Flags
        appendUint16BE(query, optionDataLen);  // RDLENGTH
        
        // RDATA: EDNS0 chunk request option
        appendUint16BE(query, EDNS0_CHUNK_REQUEST_CODE);
        appendUint16BE(query, 18);             // Option length
        query.append(chunkId.left(16).leftJustified(16, '\0')); // chunkId (16 bytes)
        appendUint16BE(query, chunkIndex);     // chunkIndex
        
        return query;
    }
    
    // Parse EDNS0 chunk metadata from DNS response
    ChunkMeta parseChunkMeta(const QByteArray &response)
    {
        ChunkMeta meta;
        meta.totalChunks = 0;
        meta.chunkIndex = 0;
        meta.totalSize = 0;
        
        if (response.size() < 12) return meta;
        
        const quint8 *data = reinterpret_cast<const quint8*>(response.constData());
        
        // Parse header
        quint16 qdCount = (data[4] << 8) | data[5];
        quint16 anCount = (data[6] << 8) | data[7];
        quint16 nsCount = (data[8] << 8) | data[9];
        quint16 arCount = (data[10] << 8) | data[11];
        
        int pos = 12;
        
        // Helper lambda to safely skip DNS name with bounds checking
        auto skipDnsName = [&]() -> bool {
            int maxLabels = 128; // Prevent infinite loops
            while (pos < response.size() && data[pos] != 0 && maxLabels-- > 0) {
                if ((data[pos] & 0xC0) == 0xC0) { 
                    pos += 2; 
                    return pos <= response.size(); 
                }
                int labelLen = data[pos];
                if (pos + 1 + labelLen > response.size()) return false; // Bounds check
                pos += labelLen + 1;
            }
            if (pos < response.size() && data[pos] == 0) pos++;
            return pos <= response.size();
        };
        
        // Skip questions
        for (int i = 0; i < qdCount && pos < response.size(); i++) {
            if (!skipDnsName()) return meta;
            if (pos + 4 > response.size()) return meta;
            pos += 4; // QTYPE + QCLASS
        }
        
        // Skip answers
        for (int i = 0; i < anCount && pos < response.size(); i++) {
            if (!skipDnsName()) return meta;
            if (pos + 10 > response.size()) return meta;
            quint16 rdlen = (data[pos + 8] << 8) | data[pos + 9];
            if (pos + 10 + rdlen > response.size()) return meta;
            pos += 10 + rdlen;
        }
        
        // Skip authority
        for (int i = 0; i < nsCount && pos < response.size(); i++) {
            if (!skipDnsName()) return meta;
            if (pos + 10 > response.size()) return meta;
            quint16 rdlen = (data[pos + 8] << 8) | data[pos + 9];
            if (pos + 10 + rdlen > response.size()) return meta;
            pos += 10 + rdlen;
        }
        
        // Parse additional (looking for OPT record)
        for (int i = 0; i < arCount && pos < response.size(); i++) {
            // Skip name
            if (pos < response.size() && data[pos] == 0) {
                pos++; // Root label for OPT
            } else {
                if (!skipDnsName()) return meta;
            }
            
            if (pos + 10 > response.size()) return meta;
            
            quint16 rtype = (data[pos] << 8) | data[pos + 1];
            quint16 rdlen = (data[pos + 8] << 8) | data[pos + 9];
            if (pos + 10 + rdlen > response.size()) return meta;
            pos += 10;
            
            if (rtype == 41 && rdlen > 0) { // OPT record
                int optEnd = pos + rdlen;
                while (pos + 4 <= optEnd) {
                    quint16 optCode = (data[pos] << 8) | data[pos + 1];
                    quint16 optLen = (data[pos + 2] << 8) | data[pos + 3];
                    pos += 4;
                    
                    if (optCode == EDNS0_CHUNK_RESPONSE_CODE && optLen >= 24) {
                        // Parse chunk metadata: chunkId(16) + total(2) + index(2) + size(4)
                        meta.chunkId = QByteArray(reinterpret_cast<const char*>(data + pos), 16);
                        meta.totalChunks = (data[pos + 16] << 8) | data[pos + 17];
                        meta.chunkIndex = (data[pos + 18] << 8) | data[pos + 19];
                        meta.totalSize = (data[pos + 20] << 24) | (data[pos + 21] << 16) | 
                                        (data[pos + 22] << 8) | data[pos + 23];
                        qDebug() << "[DNS Tunnel] Chunk meta: id=" << meta.chunkId.toHex()
                                 << "total=" << meta.totalChunks << "index=" << meta.chunkIndex
                                 << "size=" << meta.totalSize;
                        return meta;
                    }
                    pos += optLen;
                }
            } else {
                pos += rdlen;
            }
        }
        
        return meta;
    }
    
    // Build DNS TXT query with EDNS0 payload (initial request)
    QByteArray buildDnsTxtQueryWithPayload(const QString &queryName, quint16 transactionId, const QByteArray &payload)
    {
        QByteArray query;
        
        // DNS Header (12 bytes)
        appendUint16BE(query, transactionId);  // Transaction ID
        appendUint16BE(query, 0x0100);         // Flags: standard query, RD=1
        appendUint16BE(query, 1);              // Questions: 1
        appendUint16BE(query, 0);              // Answers: 0
        appendUint16BE(query, 0);              // Authority: 0
        appendUint16BE(query, 1);              // Additional: 1 (EDNS0 OPT)
        
        // Question section - QNAME
        QStringList labels = queryName.split('.');
        for (const QString &label : labels) {
            QByteArray labelBytes = label.toUtf8();
            query.append(static_cast<char>(labelBytes.size()));
            query.append(labelBytes);
        }
        query.append(static_cast<char>(0));    // Root label (end of QNAME)
        appendUint16BE(query, 16);             // QTYPE: TXT (16)
        appendUint16BE(query, 1);              // QCLASS: IN (1)
        
        // Additional section - EDNS0 OPT record
        QByteArray payloadBase64 = payload.toBase64();
        quint16 optionDataLen = 4 + payloadBase64.size(); // code(2) + length(2) + data
        
        query.append(static_cast<char>(0));    // Name: root (0)
        appendUint16BE(query, 41);             // TYPE: OPT (41)
        appendUint16BE(query, 4096);           // CLASS: UDP payload size
        query.append(static_cast<char>(0));    // Extended RCODE
        query.append(static_cast<char>(0));    // EDNS version
        appendUint16BE(query, 0);              // Flags (Z)
        appendUint16BE(query, optionDataLen);  // RDLENGTH
        
        // RDATA: EDNS0 payload option
        appendUint16BE(query, EDNS0_PAYLOAD_OPTION_CODE); // Option code
        appendUint16BE(query, payloadBase64.size());       // Option length
        query.append(payloadBase64);                       // Option data
        
        return query;
    }
    
    // Parse DNS TXT response and extract payload
    QByteArray parseDnsTxtResponse(const QByteArray &response)
    {
        if (response.size() < 12) {
            qDebug() << "[DNS Tunnel] Response too short:" << response.size();
            return QByteArray();
        }
        
        // Parse header manually to avoid QDataStream issues
        const uchar *data = reinterpret_cast<const uchar*>(response.constData());
        int pos = 0;
        
        quint16 transactionId = (data[pos] << 8) | data[pos+1]; pos += 2;
        quint16 flags = (data[pos] << 8) | data[pos+1]; pos += 2;
        quint16 qdCount = (data[pos] << 8) | data[pos+1]; pos += 2;
        quint16 anCount = (data[pos] << 8) | data[pos+1]; pos += 2;
        quint16 nsCount = (data[pos] << 8) | data[pos+1]; pos += 2;
        quint16 arCount = (data[pos] << 8) | data[pos+1]; pos += 2;
        
        qDebug() << "[DNS Tunnel] Response header: id=" << transactionId << "flags=" << Qt::hex << flags 
                 << "questions=" << qdCount << "answers=" << anCount << "authority=" << nsCount << "additional=" << arCount;
        
        // Validate QR bit - must be 1 for response (bit 15 of flags)
        if ((flags & 0x8000) == 0) {
            qDebug() << "[DNS Tunnel] Invalid response: QR bit not set (not a response)";
            return QByteArray();
        }
        
        // Check for errors (RCODE in lower 4 bits of flags)
        quint8 rcode = flags & 0x0F;
        if (rcode != 0) {
            qDebug() << "[DNS Tunnel] Response error, RCODE:" << rcode;
        }
        
        // Sanity check on counts to prevent excessive iteration
        if (anCount > 100 || qdCount > 10) {
            qDebug() << "[DNS Tunnel] Suspicious counts, likely garbage data";
            return QByteArray();
        }
        
        // Helper lambda to safely skip DNS name with bounds checking
        auto skipDnsName = [&]() -> bool {
            int maxLabels = 128; // Prevent infinite loops
            while (pos < response.size() && data[pos] != 0 && maxLabels-- > 0) {
                if ((data[pos] & 0xC0) == 0xC0) { 
                    pos += 2; 
                    return pos <= response.size(); 
                }
                int labelLen = data[pos];
                if (pos + 1 + labelLen > response.size()) return false;
                pos += labelLen + 1;
            }
            if (pos < response.size() && data[pos] == 0) pos++;
            return pos <= response.size();
        };
        
        // Skip question section
        for (int i = 0; i < qdCount && pos < response.size(); i++) {
            if (!skipDnsName()) {
                qDebug() << "[DNS Tunnel] Failed to skip question name";
                return QByteArray();
            }
            if (pos + 4 > response.size()) return QByteArray();
            pos += 4; // Skip QTYPE and QCLASS
        }
        
        qDebug() << "[DNS Tunnel] After questions, pos=" << pos << "size=" << response.size();
        
        // Read answer section - looking for TXT records
        QByteArray combinedTxt;
        for (int i = 0; i < anCount && pos < response.size(); i++) {
            if (!skipDnsName()) {
                qDebug() << "[DNS Tunnel] Failed to skip answer name at pos=" << pos;
                break;
            }
            
            if (pos + 10 > response.size()) {
                qDebug() << "[DNS Tunnel] Not enough data for RR header at pos=" << pos;
                break;
            }
            
            quint16 rtype = (data[pos] << 8) | data[pos+1]; pos += 2;
            quint16 rclass = (data[pos] << 8) | data[pos+1]; pos += 2;
            quint32 ttl = (data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3]; pos += 4;
            quint16 rdlength = (data[pos] << 8) | data[pos+1]; pos += 2;
            
            qDebug() << "[DNS Tunnel] Answer" << i << ": type=" << rtype << "class=" << rclass << "ttl=" << ttl << "rdlen=" << rdlength;
            
            // Bounds check for rdlength
            if (pos + rdlength > response.size()) {
                qDebug() << "[DNS Tunnel] rdlength exceeds buffer, truncating";
                break;
            }
            
            if (rtype == 16) { // TXT record
                int rdEnd = pos + rdlength;
                while (pos < rdEnd && pos < response.size()) {
                    quint8 txtLen = data[pos++];
                    if (txtLen > 0 && pos + txtLen <= rdEnd && pos + txtLen <= response.size()) {
                        combinedTxt.append(reinterpret_cast<const char*>(data + pos), txtLen);
                        pos += txtLen;
                    } else {
                        break; // Invalid TXT length
                    }
                }
            } else {
                pos += rdlength; // Skip non-TXT record data
            }
        }
        
        if (combinedTxt.isEmpty()) {
            qDebug() << "[DNS Tunnel] No TXT records in response";
            return QByteArray();
        }
        
        qDebug() << "[DNS Tunnel] Received TXT data:" << combinedTxt.size() << "bytes";
        
        // Decode base64
        QByteArray decoded = QByteArray::fromBase64(combinedTxt);
        qDebug() << "[DNS Tunnel] Decoded data:" << decoded.size() << "bytes, preview:" << decoded.left(100);
        return decoded;
    }
}

QByteArray NetworkUtilities::sendViaDnsTunnel(const QByteArray &payload, const QString &endpoint, const QString &baseDomain,
                                               const QString &dnsServer, DnsTransport transport, quint16 port,
                                               int timeoutMsecs, const QString &dohEndpoint)
{
    // Build query name: endpoint.baseDomain (e.g., services.gateway.example.com)
    QString queryName = QString("%1.%2").arg(endpoint, baseDomain);
    
    qDebug() << "[DNS Tunnel] Sending to" << queryName << "via" << (transport == DnsTransport::Udp ? "UDP" : 
               transport == DnsTransport::Tcp ? "TCP" : 
               transport == DnsTransport::Tls ? "DoT" : 
               transport == DnsTransport::Https ? "DoH" : "DoQ")
             << "server:" << dnsServer << "port:" << port << "payload:" << payload.size() << "bytes";
    
    switch (transport) {
        case DnsTransport::Udp:
            // Try chunked UDP first (handles large responses)
            return sendViaDnsTunnelUdpChunked(payload, queryName, dnsServer, port, timeoutMsecs);
        case DnsTransport::Tcp:
            return sendViaDnsTunnelTcp(payload, queryName, dnsServer, port, timeoutMsecs);
        case DnsTransport::Tls:
            return sendViaDnsTunnelTls(payload, queryName, dnsServer, port, timeoutMsecs);
        case DnsTransport::Https:
            return sendViaDnsTunnelHttps(payload, queryName, dnsServer, port, dohEndpoint, timeoutMsecs);
        case DnsTransport::Quic:
            // DoQ uses QUIC - not yet implemented
            qDebug() << "[DNS Tunnel] DoQ not yet implemented";
            return QByteArray(); // Return empty to trigger fallback to other transports
    }
    return QByteArray();
}

QByteArray NetworkUtilities::sendViaDnsTunnelUdp(const QByteArray &payload, const QString &queryName,
                                                  const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QUdpSocket socket;
    
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray query = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);
    
    if (query.isEmpty()) {
        qDebug() << "[DNS Tunnel UDP] Failed to build query";
        return QByteArray();
    }
    
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qDebug() << "[DNS Tunnel UDP] Invalid DNS server address:" << dnsServer;
        return QByteArray();
    }
    
    qint64 bytesWritten = socket.writeDatagram(query, dnsAddress, port);
    if (bytesWritten != query.size()) {
        qDebug() << "[DNS Tunnel UDP] Failed to send query";
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel UDP] Sent" << bytesWritten << "bytes to" << dnsServer << ":" << port;
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QUdpSocket::readyRead, [&]() {
        while (socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = socket.receiveDatagram();
            if (datagram.isValid()) {
                response = datagram.data();
                responseReceived = true;
                loop.quit();
            }
        }
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    
    if (!responseReceived || response.isEmpty()) {
        qDebug() << "[DNS Tunnel UDP] No response received (timeout)";
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel UDP] Received response:" << response.size() << "bytes";
    return parseDnsTxtResponse(response);
}

QByteArray NetworkUtilities::sendViaDnsTunnelTcp(const QByteArray &payload, const QString &queryName,
                                                  const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QTcpSocket socket;
    
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qDebug() << "[DNS Tunnel TCP] Invalid DNS server address:" << dnsServer;
        return QByteArray();
    }
    
    socket.connectToHost(dnsAddress, port);
    if (!socket.waitForConnected(timeoutMsecs)) {
        qDebug() << "[DNS Tunnel TCP] Connection failed:" << socket.errorString();
        return QByteArray();
    }
    
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray query = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);
    
    if (query.isEmpty()) {
        qDebug() << "[DNS Tunnel TCP] Failed to build query";
        socket.close();
        return QByteArray();
    }
    
    // TCP DNS: 2-byte length prefix
    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tcpQuery;
    tcpQuery.append(reinterpret_cast<const char*>(&length), sizeof(quint16));
    tcpQuery.append(query);
    
    qint64 bytesWritten = socket.write(tcpQuery);
    if (bytesWritten != tcpQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        qDebug() << "[DNS Tunnel TCP] Failed to send query";
        socket.close();
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel TCP] Sent" << bytesWritten << "bytes to" << dnsServer << ":" << port;
    
    // Wait for response
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QByteArray response;
    bool responseReceived = false;
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::readyRead, [&]() {
        if (socket.bytesAvailable() >= 2 && response.isEmpty()) {
            QByteArray lengthBytes = socket.read(2);
            if (lengthBytes.size() == 2) {
                quint16 responseLength = qFromBigEndian<quint16>(*reinterpret_cast<const quint16*>(lengthBytes.constData()));
                while (socket.bytesAvailable() < responseLength) {
                    if (!socket.waitForReadyRead(timeoutMsecs / 2)) {
                        break;
                    }
                }
                if (socket.bytesAvailable() >= responseLength) {
                    response = socket.read(responseLength);
                    responseReceived = true;
                    loop.quit();
                }
            }
        }
    });
    
    timer.start();
    loop.exec();
    timer.stop();
    socket.close();
    
    if (!responseReceived || response.isEmpty()) {
        qDebug() << "[DNS Tunnel TCP] No response received (timeout)";
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel TCP] Received response:" << response.size() << "bytes";
    return parseDnsTxtResponse(response);
}

QByteArray NetworkUtilities::sendViaDnsTunnelTls(const QByteArray &payload, const QString &queryName,
                                                  const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    QSslSocket socket;
    
    // Disable certificate verification for local testing
    socket.setPeerVerifyMode(QSslSocket::VerifyNone);
    
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qDebug() << "[DNS Tunnel DoT] Invalid DNS server address:" << dnsServer;
        return QByteArray();
    }
    
    socket.connectToHostEncrypted(dnsServer, port);
    if (!socket.waitForEncrypted(timeoutMsecs)) {
        qDebug() << "[DNS Tunnel DoT] TLS handshake failed:" << socket.errorString();
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel DoT] TLS connected to" << dnsServer << ":" << port;
    
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray query = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);
    
    if (query.isEmpty()) {
        qDebug() << "[DNS Tunnel DoT] Failed to build query";
        socket.close();
        return QByteArray();
    }
    
    // TCP DNS format: 2-byte length prefix
    quint16 length = qToBigEndian<quint16>(static_cast<quint16>(query.size()));
    QByteArray tcpQuery;
    tcpQuery.append(reinterpret_cast<const char*>(&length), sizeof(quint16));
    tcpQuery.append(query);
    
    qint64 bytesWritten = socket.write(tcpQuery);
    if (bytesWritten != tcpQuery.size() || !socket.waitForBytesWritten(timeoutMsecs)) {
        qDebug() << "[DNS Tunnel DoT] Failed to send query";
        socket.close();
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel DoT] Sent" << bytesWritten << "bytes";
    
    // Synchronous read: first get 2-byte length prefix
    QElapsedTimer timer;
    timer.start();
    
    while (socket.bytesAvailable() < 2) {
        int remaining = timeoutMsecs - timer.elapsed();
        if (remaining <= 0 || !socket.waitForReadyRead(remaining)) {
            qDebug() << "[DNS Tunnel DoT] Timeout waiting for response length";
            socket.close();
            return QByteArray();
        }
    }
    
    QByteArray lengthBytes = socket.read(2);
    if (lengthBytes.size() != 2) {
        qDebug() << "[DNS Tunnel DoT] Failed to read response length";
        socket.close();
        return QByteArray();
    }
    
    quint16 responseLength = qFromBigEndian<quint16>(*reinterpret_cast<const quint16*>(lengthBytes.constData()));
    
    // Now read the full response body
    QByteArray response;
    while (response.size() < responseLength) {
        int remaining = timeoutMsecs - timer.elapsed();
        if (remaining <= 0) {
            qDebug() << "[DNS Tunnel DoT] Timeout waiting for response body";
            socket.close();
            return QByteArray();
        }
        
        if (socket.bytesAvailable() > 0) {
            response.append(socket.read(responseLength - response.size()));
        } else if (!socket.waitForReadyRead(remaining)) {
            qDebug() << "[DNS Tunnel DoT] Timeout waiting for more data";
            socket.close();
            return QByteArray();
        }
    }
    
    socket.close();
    
    qDebug() << "[DNS Tunnel DoT] Received response:" << response.size() << "bytes";
    return parseDnsTxtResponse(response);
}

QByteArray NetworkUtilities::sendViaDnsTunnelHttps(const QByteArray &payload, const QString &queryName,
                                                    const QString &dnsServer, quint16 port, const QString &endpoint, int timeoutMsecs)
{
    // Build DNS query packet
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray dnsQuery = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);
    
    if (dnsQuery.isEmpty()) {
        qDebug() << "[DNS Tunnel DoH] Failed to build query";
        return QByteArray();
    }
    
    // DoH uses HTTP POST with application/dns-message
    // Use HTTPS for port 443, HTTP for other ports (like 80 for local testing)
    QString scheme = (port == 443) ? "https" : "http";
    QString url = QString("%1://%2:%3%4").arg(scheme).arg(dnsServer).arg(port).arg(endpoint);
    
    qDebug() << "[DNS Tunnel DoH] Sending to" << url;
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/dns-message");
    request.setRawHeader("Accept", "application/dns-message");
    request.setTransferTimeout(timeoutMsecs);
    
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(request, dnsQuery);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMsecs);
    
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    
    timer.start();
    loop.exec();
    timer.stop();
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[DNS Tunnel DoH] HTTP error:" << reply->errorString();
        reply->deleteLater();
        return QByteArray();
    }
    
    QByteArray response = reply->readAll();
    reply->deleteLater();
    
    if (response.isEmpty()) {
        qDebug() << "[DNS Tunnel DoH] Empty response";
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel DoH] Received response:" << response.size() << "bytes";
    return parseDnsTxtResponse(response);
}

QByteArray NetworkUtilities::sendViaDnsTunnelUdpChunked(const QByteArray &payload, const QString &queryName,
                                                         const QString &dnsServer, quint16 port, int timeoutMsecs)
{
    qDebug() << "[DNS Tunnel UDP Chunked] Starting request to" << queryName;
    
    QHostAddress dnsAddress(dnsServer);
    if (dnsAddress.isNull()) {
        qDebug() << "[DNS Tunnel UDP Chunked] Invalid DNS server:" << dnsServer;
        return QByteArray();
    }
    
    // Constants for retry logic
    constexpr int MAX_INITIAL_RETRIES = 3;
    constexpr int MAX_CHUNK_RETRIES = 2;
    constexpr int MAX_CONCURRENT_REQUESTS = 5;
    constexpr int BASE_TIMEOUT_MS = 2000;
    
    // Helper lambda to send UDP request with configurable timeout
    auto sendUdpRequestWithTimeout = [&](const QByteArray &query, int requestTimeoutMs) -> QByteArray {
        QUdpSocket socket;
        qint64 written = socket.writeDatagram(query, dnsAddress, port);
        if (written != query.size()) {
            return QByteArray();
        }
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(requestTimeoutMs);
        
        QByteArray response;
        bool responseReceived = false;
        
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&socket, &QUdpSocket::readyRead, [&]() {
            while (socket.hasPendingDatagrams()) {
                QNetworkDatagram datagram = socket.receiveDatagram();
                if (datagram.isValid()) {
                    response = datagram.data();
                    responseReceived = true;
                    loop.quit();
                }
            }
        });
        
        timer.start();
        loop.exec();
        timer.stop();
        
        return responseReceived ? response : QByteArray();
    };
    
    // Helper lambda with retry and exponential backoff
    auto sendWithRetry = [&](const QByteArray &query, int maxRetries) -> QByteArray {
        for (int attempt = 0; attempt < maxRetries; attempt++) {
            int timeout = BASE_TIMEOUT_MS * (attempt + 1); // 2s, 4s, 6s
            qDebug() << "[DNS Tunnel UDP Chunked] Attempt" << (attempt + 1) << "/" << maxRetries 
                     << "timeout:" << timeout << "ms";
            
            QByteArray response = sendUdpRequestWithTimeout(query, timeout);
            if (!response.isEmpty()) {
                return response;
            }
            
            if (attempt < maxRetries - 1) {
                qDebug() << "[DNS Tunnel UDP Chunked] Retry after" << (timeout / 2) << "ms";
                QThread::msleep(timeout / 2);
            }
        }
        return QByteArray();
    };
    
    // === Step 1: Send initial request with retry ===
    quint16 transactionId = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    QByteArray initialQuery = buildDnsTxtQueryWithPayload(queryName, transactionId, payload);
    
    if (initialQuery.isEmpty()) {
        qDebug() << "[DNS Tunnel UDP Chunked] Failed to build initial query";
        return QByteArray();
    }
    
    qDebug() << "[DNS Tunnel UDP Chunked] Sending initial request, payload:" << payload.size() << "bytes";
    QByteArray firstResponse = sendWithRetry(initialQuery, MAX_INITIAL_RETRIES);
    
    if (firstResponse.isEmpty()) {
        qDebug() << "[DNS Tunnel UDP Chunked] No response for initial request after" << MAX_INITIAL_RETRIES << "attempts";
        return QByteArray();
    }
    
    // Parse chunk metadata from EDNS0 option 65003
    ChunkMeta meta = parseChunkMeta(firstResponse);
    QByteArray firstTxtData = parseDnsTxtResponse(firstResponse);
    
    if (firstTxtData.isEmpty()) {
        qDebug() << "[DNS Tunnel UDP Chunked] Failed to parse TXT response";
        return QByteArray();
    }
    
    // Check if response is chunked
    if (meta.totalChunks <= 1) {
        qDebug() << "[DNS Tunnel UDP Chunked] Single chunk response:" << firstTxtData.size() << "bytes";
        return firstTxtData;
    }
    
    qDebug() << "[DNS Tunnel UDP Chunked] Chunked response: total=" << meta.totalChunks 
             << "size=" << meta.totalSize << "chunkId=" << meta.chunkId.toHex();
    
    // === Step 2: Collect all chunks ===
    QMap<int, QByteArray> chunks;
    chunks[0] = firstTxtData;
    
    // Build list of chunks to request
    QList<int> chunksToRequest;
    for (int i = 1; i < meta.totalChunks; i++) {
        chunksToRequest.append(i);
    }
    
    // === Step 3: Request chunks in parallel batches with retry ===
    auto requestChunksBatch = [&](const QList<int> &chunkIndices, int batchTimeout) {
        if (chunkIndices.isEmpty()) return;
        
        // Create sockets and send requests for this batch
        QList<QSharedPointer<QUdpSocket>> sockets;
        QMap<QUdpSocket*, int> socketToIndex;
        
        for (int idx : chunkIndices) {
            if (chunks.contains(idx)) continue; // Already have this chunk
            
            quint16 chunkTxId = static_cast<quint16>((QDateTime::currentMSecsSinceEpoch() + idx) & 0xFFFF);
            QByteArray chunkQuery = buildDnsChunkRequest(queryName, chunkTxId, meta.chunkId, idx);
            
            if (chunkQuery.isEmpty()) {
                qDebug() << "[DNS Tunnel UDP Chunked] Failed to build chunk request" << idx;
                continue;
            }
            
            auto socket = QSharedPointer<QUdpSocket>::create();
            socket->writeDatagram(chunkQuery, dnsAddress, port);
            socketToIndex[socket.data()] = idx;
            sockets.append(socket);
        }
        
        if (sockets.isEmpty()) return;
        
        qDebug() << "[DNS Tunnel UDP Chunked] Sent" << sockets.size() << "parallel requests";
        
        // Wait for responses with deadline
        QEventLoop loop;
        QTimer deadline;
        deadline.setSingleShot(true);
        deadline.setInterval(batchTimeout);
        
        int receivedCount = 0;
        int expectedCount = sockets.size();
        
        QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        
        for (auto &socket : sockets) {
            QObject::connect(socket.data(), &QUdpSocket::readyRead, [&, sock = socket.data()]() {
                while (sock->hasPendingDatagrams()) {
                    QNetworkDatagram datagram = sock->receiveDatagram();
                    if (datagram.isValid()) {
                        QByteArray chunkTxtData = parseDnsTxtResponse(datagram.data());
                        if (!chunkTxtData.isEmpty()) {
                            ChunkMeta chunkMeta = parseChunkMeta(datagram.data());
                            int idx = (chunkMeta.totalChunks > 0) ? chunkMeta.chunkIndex : socketToIndex.value(sock, -1);
                            if (idx >= 0 && !chunks.contains(idx)) {
                                chunks[idx] = chunkTxtData;
                                qDebug() << "[DNS Tunnel UDP Chunked] Received chunk" << idx << ":" << chunkTxtData.size() << "bytes";
                                receivedCount++;
                            }
                        }
                    }
                }
                
                // Exit early if we have all chunks
                if (receivedCount >= expectedCount || chunks.size() >= meta.totalChunks) {
                    loop.quit();
                }
            });
        }
        
        deadline.start();
        loop.exec();
        deadline.stop();
    };
    
    // Process chunks in batches
    int totalTimeout = qMax(timeoutMsecs / 2, 5000); // At least 5 seconds for chunks
    int batchTimeout = totalTimeout / (MAX_CHUNK_RETRIES + 1);
    
    for (int retryRound = 0; retryRound <= MAX_CHUNK_RETRIES; retryRound++) {
        // Find missing chunks
        QList<int> missing;
        for (int i = 1; i < meta.totalChunks; i++) {
            if (!chunks.contains(i)) {
                missing.append(i);
            }
        }
        
        if (missing.isEmpty()) {
            qDebug() << "[DNS Tunnel UDP Chunked] All chunks received";
            break;
        }
        
        if (retryRound > 0) {
            qDebug() << "[DNS Tunnel UDP Chunked] Retry round" << retryRound << "for" << missing.size() << "missing chunks";
        }
        
        // Process in batches of MAX_CONCURRENT_REQUESTS
        for (int batchStart = 0; batchStart < missing.size(); batchStart += MAX_CONCURRENT_REQUESTS) {
            QList<int> batch = missing.mid(batchStart, MAX_CONCURRENT_REQUESTS);
            requestChunksBatch(batch, batchTimeout);
        }
    }
    
    // === Step 4: Verify all chunks received ===
    QList<int> finalMissing;
    for (int i = 0; i < meta.totalChunks; i++) {
        if (!chunks.contains(i)) {
            finalMissing.append(i);
        }
    }
    
    if (!finalMissing.isEmpty()) {
        qDebug() << "[DNS Tunnel UDP Chunked] FAILED: Missing chunks after all retries:" << finalMissing;
        qDebug() << "[DNS Tunnel UDP Chunked] Received" << chunks.size() << "/" << meta.totalChunks << "chunks";
        return QByteArray(); // Return empty - don't return partial/corrupted data
    }
    
    // === Step 5: Combine all chunks in order ===
    QByteArray combined;
    combined.reserve(meta.totalSize > 0 ? meta.totalSize : meta.totalChunks * 500);
    
    for (int i = 0; i < meta.totalChunks; i++) {
        combined.append(chunks[i]);
    }
    
    qDebug() << "[DNS Tunnel UDP Chunked] SUCCESS: Combined" << meta.totalChunks << "chunks," << combined.size() << "bytes";
    return combined;
}
