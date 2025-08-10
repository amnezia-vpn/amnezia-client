#include "router_linux.h"

#include <QProcess>
#include <QThread>
#include <utilities.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <paths.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <QFileInfo>

#include <core/networkUtilities.h>

RouterLinux &RouterLinux::Instance()
{
    static RouterLinux s;
    return s;
}

bool RouterLinux::routeAdd(const QString &ipWithSubnet, const QString &gw, const int &sock)
{
    QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = NetworkUtilities::netMaskFromIpWithSubnet(ipWithSubnet);

    if (!NetworkUtilities::checkIPv4Format(ip) || !NetworkUtilities::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to add invalid route: " << ip << gw;
        return false;
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
        qDebug().noquote() << "route add error: gw "
                           << ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr
                           << " ip " << ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr
                           << " mask " << ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr << " " << err;
        return false;
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
    qDebug().noquote() << "RouterLinux::routeDelete: " << ipWithSubnet << gw;
#endif

    QString ip = NetworkUtilities::ipAddressFromIpWithSubnet(ipWithSubnet);
    QString mask = NetworkUtilities::netMaskFromIpWithSubnet(ipWithSubnet);

    if (!NetworkUtilities::checkIPv4Format(ip) || !NetworkUtilities::checkIPv4Format(gw)) {
        qCritical().noquote() << "Critical, trying to remove invalid route: " << ip << gw;
        return false;
    }

    if (ipWithSubnet == "0.0.0.0/0") {
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
        qDebug().noquote() << "route delete error: gw " << gw << " ip " << ip << " mask " << mask;
        return false;
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

bool RouterLinux::isServiceActive(const QString &serviceName) {
    QProcess process;
    process.start("systemctl", { "is-active", "--quiet", serviceName });
    process.waitForFinished();

    return process.exitCode() == 0;
}

void RouterLinux::flushDns()
{
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);

    //check what the dns manager use
    if (isServiceActive("nscd.service")) {
        qDebug() << "Restarting nscd.service";
        p.start("systemctl", { "restart", "nscd" });
    } else if (isServiceActive("systemd-resolved.service")) {
        qDebug() << "Restarting systemd-resolved.service";
        p.start("systemctl", { "restart", "systemd-resolved" });
    } else {
        qDebug() << "No suitable DNS manager found.";
        return;
    }

    p.waitForFinished();
    QByteArray output(p.readAll());
    if (output.isEmpty())
        qDebug().noquote() << "Flush dns completed";
    else
        qDebug().noquote() << "OUTPUT systemctl restart nscd/systemd-resolved: " + output;
}

bool RouterLinux::createTun(const QString &dev, const QString &subnet) {
    qDebug().noquote() << "createTun start";

    QProcess process;
    QStringList commands;

    commands << "ip" << "tuntap" << "add" << "mode" << "tun" << "dev" << dev;
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start adding tun device!\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not add tun device!\n";
        return false;
    }
    commands.clear();

    commands << "ip" << "addr" << "add" << QString("%1/24").arg(subnet) << "dev" << dev;
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start adding a subnet for tun device!\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not add a subnet for tun device!\n";
        return false;
    }
    commands.clear();

    commands << "ip" << "link" << "set" << "dev" << dev << "up";
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start link set for tun device!\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not link set for tun device!\n";
        return false;
    }

    return true;
}

bool RouterLinux::deleteTun(const QString &dev)
{
    struct {
        struct nlmsghdr  nh;
        struct ifinfomsg ifm;
        unsigned char    data[64];
    } req;
    struct rtattr *rta;
    int ret, rtnl;

    rtnl = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (rtnl < 0) {
        qDebug().noquote() << "can't open rtnl: " << errno;
        return 1;
    }

    memset(&req, 0, sizeof(req));
    req.nh.nlmsg_len = NLMSG_ALIGN(NLMSG_LENGTH(sizeof(req.ifm)));
    req.nh.nlmsg_flags = NLM_F_REQUEST;
    req.nh.nlmsg_type = RTM_DELLINK;

    req.ifm.ifi_family = AF_UNSPEC;

    rta = (struct rtattr *)(((char *)&req) + NLMSG_ALIGN(req.nh.nlmsg_len));
    rta->rta_type = IFLA_IFNAME;
    rta->rta_len = RTA_LENGTH(IFNAMSIZ);
    req.nh.nlmsg_len += rta->rta_len;
    memcpy(RTA_DATA(rta), dev.toStdString().c_str(), IFNAMSIZ);

    ret = send(rtnl, &req, req.nh.nlmsg_len, 0);
    if (ret < 0)
        qDebug().noquote() << "can't send: errno";
    ret = (unsigned int)ret != req.nh.nlmsg_len;

    close(rtnl);
    qDebug().noquote() << "deleteTun ret" << ret;
    return ret;
}

bool RouterLinux::updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers)
{
    return m_dnsUtil->updateResolvers(ifname, resolvers);
}

void RouterLinux::StartRoutingIpv6()
{
    QProcess process;
    QStringList commands;

    commands << "sysctl" << "-w" << "net.ipv6.conf.all.disable_ipv6=0";
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start activate ipv6\n";
        return;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not activate ipv6\n";
        return;
    }
    commands.clear();

    commands << "sysctl" << "-w" << "net.ipv6.conf.default.disable_ipv6=0";
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start activate ipv6\n";
        return;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not activate ipv6\n";
        return;
    }
    commands.clear();

    qDebug().noquote() << "StartRoutingIpv6 OK";
}

void RouterLinux::StopRoutingIpv6()
{
    QProcess process;
    QStringList commands;

    commands << "sysctl" << "-w" << "net.ipv6.conf.all.disable_ipv6=1";
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start disable ipv6\n";
        return;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not disable ipv6\n";
        return;
    }
    commands.clear();

    commands << "sysctl" << "-w" << "net.ipv6.conf.default.disable_ipv6=1";
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start disable ipv6\n";
        return;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not disable ipv6\n";
        return;
    }
    commands.clear();

    qDebug().noquote() << "StopRoutingIpv6 OK";
}
