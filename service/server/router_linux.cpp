#include "router_linux.h"

#include <QProcess>
#include <QThread>
#include <utilities.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <paths.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>

#include <QFileInfo>

RouterLinux &RouterLinux::Instance()
{
    static RouterLinux s;
    return s;
}

#define BUFFER_SIZE 4096

QString RouterLinux::getgatewayandiface()
{
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
            qDebug().noquote() << "Gateway " << gateway_address << " for interface " << interface;
            break;
        }
    }
    close(sock);
    return gateway_address;
}


bool RouterLinux::routeAdd(const QString &ipWithSubnet, const QString &gw, const int &sock)
{
    QString ip = Utils::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = Utils::netMaskFromIpWithSubnet(ipWithSubnet);

    if (!Utils::checkIPv4Format(ip) || !Utils::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to add invalid route: " << ip << gw;
        return true;
    }

    struct rtentry route;
    memset(&route, 0, sizeof( route ));

    // set gateway
    ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr(gw.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_gateway)->sin_port = 0;
    // set host rejecting
    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(ip.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;
    // set mask
    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(mask.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;

    route.rt_flags = RTF_UP | RTF_GATEWAY;
    route.rt_metric = 0;

    if (int err = ioctl(sock, SIOCADDRT, &route) < 0)
    {
 //       qDebug().noquote() << "route add error: gw "
 //       << ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr
 //       << " ip " << ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr
 //       << " mask " << ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr << " " << err;
 //       return false;
    }

    m_addedRoutes.append({ipWithSubnet, gw});
    return true;
}

int RouterLinux::routeAddList(const QString &gw, const QStringList &ips)
{
    int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);
    int cnt = 0;
    for (const QString &ip: ips) {
        if (routeAdd(ip, gw, temp_sock)) cnt++;
    }
    close(temp_sock);
    return cnt;
}

bool RouterLinux::clearSavedRoutes()
{
    int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);
    int cnt = 0;
    for (const Route &r: m_addedRoutes) {
        if (routeDelete(r.dst, r.gw, temp_sock)) cnt++;
    }
    bool ret = (cnt == m_addedRoutes.count());
    m_addedRoutes.clear();
    close(temp_sock);
    return ret;
}

bool RouterLinux::routeDelete(const QString &ipWithSubnet, const QString &gw, const int &sock)
{
#ifdef MZ_DEBUG
    qDebug().noquote() << "RouterMac::routeDelete: " << ipWithSubnet << gw;
#endif

    QString ip = Utils::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = Utils::netMaskFromIpWithSubnet(ipWithSubnet);

    if (!Utils::checkIPv4Format(ip) || !Utils::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to remove invalid route: " << ip << gw;
        return true;
    }

    if (ip == "0.0.0.0") {
        qDebug().noquote() << "Warning, trying to remove default route, skipping: " << ip << gw;
        return true;
    }

    struct rtentry route;
    memset(&route, 0, sizeof( route ));

    // set gateway
    ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr(gw.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_gateway)->sin_port = 0;
    // set host rejecting
    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(ip.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;
    // set mask
    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(mask.toStdString().c_str());
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;

    route.rt_flags = RTF_UP | RTF_GATEWAY;
    route.rt_metric = 0;
    //route.rt_dev = "ens33";

    if (ioctl(sock, SIOCDELRT, &route) < 0)
    {
 //       qDebug().noquote() << "route delete error: gw " << gw << " ip " << ip << " mask " << mask;
 //       return false;
    }
    return true;
}

bool RouterLinux::routeDeleteList(const QString &gw, const QStringList &ips)
{
    int temp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    int cnt = 0;
    for (const QString &ip: ips) {
        if (routeDelete(ip, gw, temp_sock)) cnt++;
    }
    close(temp_sock);
    return cnt;
}

void RouterLinux::flushDns()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    //check what the dns manager use
    if (QFileInfo::exists("/usr/bin/nscd")
        || QFileInfo::exists("/usr/sbin/nscd")
        || QFileInfo::exists("/usr/lib/systemd/system/nscd.service"))
    {
        p.start("systemctl restart nscd");
    }
    else
    {
        p.start("systemctl restart systemd-resolved");
    }

    p.waitForFinished();
    QByteArray output(p.readAll());
    if (output.isEmpty())
        qDebug().noquote() << "Flush dns completed";
    else
        qDebug().noquote() << "OUTPUT systemctl restart nscd/systemd-resolved: " + output;
}
