#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QHostAddress>
#include <QHostInfo>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

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

#include "utilities.h"
#include "version.h"

QString Utils::getRandomString(int len)
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;
    for (int i = 0; i < len; ++i) {
        quint32 index = QRandomGenerator::global()->generate() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}

QString Utils::systemLogPath()
{
#ifdef Q_OS_WIN
    QStringList locationList = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QString primaryLocation = "ProgramData";
    foreach (const QString &location, locationList) {
        if (location.contains(primaryLocation)) {
            return QString("%1/%2/log").arg(location).arg(APPLICATION_NAME);
        }
    }
    return QString();
#else
    return QString("/var/log/%1").arg(APPLICATION_NAME);
#endif
}

bool Utils::initializePath(const QString &path)
{
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning().noquote() << QString("Cannot initialize path: '%1'").arg(path);
        return false;
    }
    return true;
}

bool Utils::createEmptyFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

QString Utils::executable(const QString &baseName, bool absPath)
{
    QString ext;
#ifdef Q_OS_WIN
    ext = ".exe";
#endif
    const QString fileName = baseName + ext;
    if (!absPath) {
        return fileName;
    }
    return QCoreApplication::applicationDirPath() + "/" + fileName;
}

QString Utils::usrExecutable(const QString &baseName)
{
    if (QFileInfo::exists("/usr/sbin/" + baseName))
        return ("/usr/sbin/" + baseName);
    else
        return ("/usr/bin/" + baseName);
}

bool Utils::processIsRunning(const QString &fileName)
{
#ifdef Q_OS_WIN
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("wmic.exe",
                  QStringList() << "/OUTPUT:STDOUT"
                                << "PROCESS"
                                << "get"
                                << "Caption");
    process.waitForStarted();
    process.waitForFinished();
    QString processData(process.readAll());
    QStringList processList = processData.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    foreach (const QString &rawLine, processList) {
        const QString line = rawLine.simplified();
        if (line.isEmpty()) {
            continue;
        }

        if (line == fileName) {
            return true;
        }
    }
    return false;
#elif defined(Q_OS_IOS)
    return false;
#else
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("pgrep", QStringList({ fileName }));
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit) {
        return (process.readAll().toUInt() > 0);
    }
    return false;
#endif
}

QString Utils::getIPAddress(const QString &host)
{
    if (ipAddressRegExp().match(host).hasMatch()) {
        return host;
    }

    QList<QHostAddress> addresses = QHostInfo::fromName(host).addresses();
    if (!addresses.isEmpty()) {
        return addresses.first().toString();
    }
    qDebug() << "Unable to resolve address for " << host;
    return "";
}

QString Utils::getStringBetween(const QString &s, const QString &a, const QString &b)
{
    int ap = s.indexOf(a), bp = s.indexOf(b, ap + a.length());
    if (ap < 0 || bp < 0)
        return QString();
    ap += a.length();
    if (bp - ap <= 0)
        return QString();
    return s.mid(ap, bp - ap).trimmed();
}

bool Utils::checkIPv4Format(const QString &ip)
{
    if (ip.isEmpty())
        return false;
    int count = ip.count(".");
    if (count != 3)
        return false;

    QHostAddress addr(ip);
    return (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol);
}

bool Utils::checkIpSubnetFormat(const QString &ip)
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

static const char* inet_ntop_v4(const void* src, char* dst, socklen_t size) {
    const char digits[] = "0123456789";
    int i;
    struct in_addr* addr = (struct in_addr*)src;
    u_long a = ntohl(addr->s_addr);
    const char* orig_dst = dst;

    if (size < 16) {
        // xerror("ENOSPC: size = %0", size);
        return NULL;
    }
    for (i = 0; i < 4; ++i) {
        int n = (a >> (24 - i * 8)) & 0xFF;
        int non_zerop = 0;

        if (non_zerop || n / 100 > 0) {
            *dst++ = digits[n / 100];
            n %= 100;
            non_zerop = 1;
        }
        if (non_zerop || n / 10 > 0) {
            *dst++ = digits[n / 10];
            n %= 10;
            non_zerop = 1;
        }
        *dst++ = digits[n];
        if (i != 3)
            *dst++ = '.';
    }
    *dst++ = '\0';
    return orig_dst;
}

// Helper function for inet_ntop for IPv6 addresses.
static const char* inet_ntop_v6(const void* src, char* dst, socklen_t size) {
    if (size < INET6_ADDRSTRLEN) {
        return NULL;
    }
    const uint16_t* as_shorts = reinterpret_cast<const uint16_t*>(src);
    int runpos[8] = {0};
    int current = 1;
    int max = 1;
    int maxpos = -1;
    int run_array_size = sizeof(runpos) / sizeof(runpos[0]);
    // Run over the address marking runs of 0s.
    for (int i = 0; i < run_array_size; ++i) {
        if (as_shorts[i] == 0) {
            runpos[i] = current;
            if (current > max) {
                maxpos = i;
                max = current;
            }
            ++current;
        } else {
            runpos[i] = -1;
            current = 1;
        }
    }
    if (max > 1) {
        int tmpmax = maxpos;
        // Run back through, setting -1 for all but the longest run.
        for (int i = run_array_size - 1; i >= 0; i--) {
            if (i > tmpmax) {
                runpos[i] = -1;
            } else if (runpos[i] == -1) {
                // We're less than maxpos, we hit a -1, so the 'good' run is done.
                // Setting tmpmax -1 means all remaining positions get set to -1.
                tmpmax = -1;
            }
        }
    }
    char* cursor = dst;
    // Print IPv4 compatible and IPv4 mapped addresses using the IPv4 helper.
    // These addresses have an initial run of either eight zero-bytes followed
    // by 0xFFFF, or an initial run of ten zero-bytes.
    if (runpos[0] == 1 && (maxpos == 5 || (maxpos == 4 && as_shorts[5] == 0xFFFF))) {
        *cursor++ = ':';
        *cursor++ = ':';
        if (maxpos == 4) {
            cursor += snprintf(cursor, INET6_ADDRSTRLEN - 2, "ffff:");
        }
        const struct in_addr* as_v4 = reinterpret_cast<const struct in_addr*>(&(as_shorts[6]));
        inet_ntop_v4(as_v4, cursor, static_cast<socklen_t>(INET6_ADDRSTRLEN - (cursor - dst)));
    } else {
        for (int i = 0; i < run_array_size; ++i) {
            if (runpos[i] == -1) {
                cursor += snprintf(cursor, INET6_ADDRSTRLEN - (cursor - dst), "%x", ntohs(as_shorts[i]));
                if (i != 7 && runpos[i + 1] != 1) {
                    *cursor++ = ':';
                }
            } else if (runpos[i] == 1) {
                // Entered the run; print the colons and skip the run.
                *cursor++ = ':';
                *cursor++ = ':';
                i += (max - 1);
            }
        }
    }
    return dst;
}

const char* socket_inet_ntop(int af, const void* src, char* dst, unsigned int size) {
    // if (IsWindows7OrGreater()){
    //   return inet_ntop(af, (PVOID)src, dst, size);
    // }

    // for OS below WINDOWS 7
    switch (af) {
    case AF_INET:
        return inet_ntop_v4(src, dst, size);
    case AF_INET6:
        return inet_ntop_v6(src, dst, size);
    default:
        // xerror("EAFNOSUPPORT");
        return NULL;
    }
}

#define NS_INADDRSZ 4

static int socket_inet_pton4(const char* src, void* dst) {
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
    unsigned char tmp[NS_INADDRSZ], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        const char* pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            size_t newNum = *tp * 10 + (pch - digits);

            if (newNum > 255)
                return (0);
            *tp = (unsigned char)newNum;
            if (!saw_digit) {
                if (++octets > 4)
                    return (0);
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return (0);
            *++tp = 0;
            saw_digit = 0;
        } else
            return (0);
    }
    if (octets < 4)
        return (0);
    memcpy(dst, tmp, NS_INADDRSZ);
    return (1);
}
// https://chromium.googlesource.com/external/webrtc/+/edc6e57a92d2b366871f4c2d2e926748326017b9/webrtc/base/win32.cc
// Helper function for inet_pton for IPv6 addresses.
static int socket_inet_pton6(const char* src, void* dst) {
    // sscanf will pick any other invalid chars up, but it parses 0xnnnn as hex.
    // Check for literal x in the input string.
    const char* readcursor = src;
    char c = *readcursor++;
    while (c) {
        if (c == 'x') {
            return 0;
        }
        c = *readcursor++;
    }
    readcursor = src;
    struct in6_addr an_addr;
    memset(&an_addr, 0, sizeof(an_addr));
    uint16_t* addr_cursor = (uint16_t*)(&an_addr.s6_addr[0]);
    uint16_t* addr_end = (uint16_t*)(&an_addr.s6_addr[16]);
    int seencompressed = 0;  // false c89 not define bool type
    // Addresses that start with "::" (i.e., a run of initial zeros) or
    // "::ffff:" can potentially be IPv4 mapped or compatibility addresses.
    // These have dotted-style IPv4 addresses on the end (e.g. "::192.168.7.1").
    if (*readcursor == ':' && *(readcursor + 1) == ':' && *(readcursor + 2) != 0) {
        // Check for periods, which we'll take as a sign of v4 addresses.
        const char* addrstart = readcursor + 2;
        if (strchr(addrstart, '.')) {
            const char* colon = strchr(addrstart, ':');
            if (colon) {
                uint16_t a_short;
                int bytesread = 0;
                if (sscanf(addrstart, "%hx%n", &a_short, &bytesread) != 1 || a_short != 0xFFFF || bytesread != 4) {
                    // Colons + periods means has to be ::ffff:a.b.c.d. But it wasn't.
                    return 0;
                } else {
                    an_addr.s6_addr[10] = 0xFF;
                    an_addr.s6_addr[11] = 0xFF;
                    addrstart = colon + 1;
                }
            }
            struct in_addr v4;
            if (socket_inet_pton4(addrstart, &v4.s_addr)) {
                memcpy(&an_addr.s6_addr[12], &v4, sizeof(v4));
                memcpy(dst, &an_addr, sizeof(an_addr));
                return 1;
            } else {
                // Invalid v4 address.
                return 0;
            }
        }
    }
    // For addresses without a trailing IPv4 component ('normal' IPv6 addresses).
    while (*readcursor != 0 && addr_cursor < addr_end) {
        if (*readcursor == ':') {
            if (*(readcursor + 1) == ':') {
                if (seencompressed) {
                    // Can only have one compressed run of zeroes ("::") per address.
                    return 0;
                }
                // Hit a compressed run. Count colons to figure out how much of the
                // address is skipped.
                readcursor += 2;
                const char* coloncounter = readcursor;
                int coloncount = 0;
                if (*coloncounter == 0) {
                    // Special case - trailing ::.
                    addr_cursor = addr_end;
                } else {
                    while (*coloncounter) {
                        if (*coloncounter == ':') {
                            ++coloncount;
                        }
                        ++coloncounter;
                    }
                    // (coloncount + 1) is the number of shorts left in the address.
                    addr_cursor = addr_end - (coloncount + 1);
                    seencompressed = 1;
                }
            } else {
                ++readcursor;
            }
        } else {
            uint16_t word;
            int bytesread = 0;
            if (sscanf(readcursor, "%hx%n", &word, &bytesread) != 1) {
                return 0;
            } else {
                *addr_cursor = htons(word);
                ++addr_cursor;
                readcursor += bytesread;
                if (*readcursor != ':' && *readcursor != '\0') {
                    return 0;
                }
            }
        }
    }
    if (*readcursor != '\0' || addr_cursor < addr_end) {
        // Catches addresses too short or too long.
        return 0;
    }
    memcpy(dst, &an_addr, sizeof(an_addr));
    return 1;
}

int socket_inet_pton(int af, const char* src, void* dst) {
    // if (IsWindows7OrGreater()){
    //   return inet_pton(af, src, dst);
    // }

    // for OS below WINDOWS 7
    switch (af) {
    case AF_INET:
        return socket_inet_pton4(src, dst);
    case AF_INET6:
        return socket_inet_pton6(src, dst);
    default:
        // xerror("EAFNOSUPPORT");
        return 0;
    }
}
#endif


QString Utils::getgatewayandiface()
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
                QString gw = socket_inet_ntop(AF_INET, &(sa_in->sin_addr), buff, BUFF_LEN);
                qDebug() <<  "gateway IPV4:" << gw;
                struct sockaddr_in addr;
                if (socket_inet_pton(AF_INET, buff, &addr.sin_addr) == 1) {
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

void Utils::killProcessByName(const QString &name)
{
    qDebug().noquote() << "Kill process" << name;
#ifdef Q_OS_WIN
    QProcess::execute("taskkill", QStringList() << "/IM" << name << "/F");
#elif defined Q_OS_IOS
    return;
#else
    QProcess::execute(QString("pkill %1").arg(name));
#endif
}

QString Utils::netMaskFromIpWithSubnet(const QString ip)
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

QString Utils::ipAddressFromIpWithSubnet(const QString ip)
{
    if (ip.count(".") != 3)
        return "";
    return ip.split("/").first();
}

QStringList Utils::summarizeRoutes(const QStringList &ips, const QString cidr)
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

QString Utils::openVpnExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable("openvpn/openvpn", true);
#elif defined Q_OS_LINUX
    // We have service that runs OpenVPN on Linux. We need to make same
    // path for client and service.
    return Utils::executable("../../client/bin/openvpn", true);
#else
    return Utils::executable("/openvpn", true);
#endif
}

QString Utils::wireguardExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable("wireguard/wireguard-service", true);
#elif defined Q_OS_LINUX
    return Utils::usrExecutable("wg-quick");
#elif defined Q_OS_MAC
    return Utils::executable("/wireguard", true);
#else
    return {};
#endif
}

QString Utils::certUtilPath()
{
#ifdef Q_OS_WIN
    QString winPath = QString::fromUtf8(qgetenv("windir"));
    return winPath + "\\system32\\certutil.exe";
#else
    return "";
#endif
}

QString Utils::tun2socksPath()
{
#ifdef Q_OS_WIN
    return Utils::executable("xray/tun2socks", true);
#elif defined Q_OS_LINUX
    // We have service that runs OpenVPN on Linux. We need to make same
    // path for client and service.
    return Utils::executable("../../client/bin/tun2socks", true);
#else
    return Utils::executable("/tun2socks", true);
#endif
}

#ifdef Q_OS_WIN
// Inspired from http://stackoverflow.com/a/15281070/1529139
// and http://stackoverflow.com/q/40059902/1529139
bool Utils::signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent)
{
    bool success = false;
    DWORD thisConsoleId = GetCurrentProcessId();
    // Leave current console if it exists
    // (otherwise AttachConsole will return ERROR_ACCESS_DENIED)
    bool consoleDetached = (FreeConsole() != FALSE);

    if (AttachConsole(dwProcessId) != FALSE) {
        // Add a fake Ctrl-C handler for avoid instant kill is this console
        // WARNING: do not revert it or current program will be also killed
        SetConsoleCtrlHandler(nullptr, true);
        success = (GenerateConsoleCtrlEvent(dwCtrlEvent, 0) != FALSE);
        FreeConsole();
    }

    if (consoleDetached) {
        // Create a new console if previous was deleted by OS
        if (AttachConsole(thisConsoleId) == FALSE) {
            int errorCode = GetLastError();
            if (errorCode == 31) // 31=ERROR_GEN_FAILURE
            {
                AllocConsole();
            }
        }
    }
    return success;
}

#endif
