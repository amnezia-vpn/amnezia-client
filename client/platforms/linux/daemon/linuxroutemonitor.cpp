/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxroutemonitor.h"

#include <QNetworkInterface>
#include <QCoreApplication>
#include <QProcess>
#include <QScopeGuard>
#include <QTimer>

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("LinuxRouteMonitor");
}  // namespace


typedef struct wg_allowedip {
    uint16_t family;
    union {
        struct in_addr ip4;
        struct in6_addr ip6;
    };
    uint8_t cidr;
    struct wg_allowedip *next_allowedip;
} wg_allowedip;

constexpr const char* WG_INTERFACE = "amn0";

static void nlmsg_append_attr(struct nlmsghdr* nlmsg, size_t maxlen,
                              int attrtype, const void* attrdata,
                              size_t attrlen);
static void nlmsg_append_attr32(struct nlmsghdr* nlmsg, size_t maxlen,
                                int attrtype, uint32_t value);

static bool buildAllowedIp(wg_allowedip* ip, const IPAddress& prefix);


LinuxRouteMonitor::LinuxRouteMonitor(const QString& ifname, QObject* parent)
    : QObject(parent), m_ifname(ifname) {
  MZ_COUNT_CTOR(LinuxRouteMonitor);
  logger.debug() << "LinuxRouteMonitor created.";

  m_nlsock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (m_nlsock < 0) {
      logger.warning() << "Failed to create netlink socket:" << strerror(errno);
  }

  struct sockaddr_nl nladdr;
  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;
  nladdr.nl_pid = getpid();
  if (bind(m_nlsock, (struct sockaddr*)&nladdr, sizeof(nladdr)) != 0) {
      logger.warning() << "Failed to bind netlink socket:" << strerror(errno);
  }

  m_notifier = new QSocketNotifier(m_nlsock, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &LinuxRouteMonitor::nlsockReady);
}

LinuxRouteMonitor::~LinuxRouteMonitor() {
  MZ_COUNT_DTOR(LinuxRouteMonitor);
  if (m_nlsock >= 0) {
      close(m_nlsock);
  }
  logger.debug() << "WireguardUtilsLinux destroyed.";
}

// Compare memory against zero.
static int memcmpzero(const void* data, size_t len) {
  const quint8* ptr = static_cast<const quint8*>(data);
  while (len--) {
    if (*ptr++) return 1;
  }
  return 0;
}

bool LinuxRouteMonitor::insertRoute(const IPAddress& prefix) {
    logger.debug() << "Adding route to" << prefix.toString();

    const int flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    return rtmSendRoute(RTM_NEWROUTE, flags, RTN_UNICAST, prefix);
}

bool LinuxRouteMonitor::deleteRoute(const IPAddress& prefix) {
    logger.debug() << "Removing route to" << prefix.toString();

    const int flags = NLM_F_REQUEST | NLM_F_ACK;
    return rtmSendRoute(RTM_DELROUTE, flags, RTN_UNICAST, prefix);
}

bool LinuxRouteMonitor::addExclusionRoute(const IPAddress& prefix) {
    logger.debug() << "Adding exclusion route for"
                   << prefix.toString();
    const int flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    return rtmSendRoute(RTM_NEWROUTE, flags, RTN_THROW, prefix);
}

bool LinuxRouteMonitor::deleteExclusionRoute(const IPAddress& prefix) {
    logger.debug() << "Removing exclusion route for"
                   << prefix.toString();
    const int flags = NLM_F_REQUEST | NLM_F_ACK;
    return rtmSendRoute(RTM_DELROUTE, flags, RTN_THROW, prefix);
}

bool LinuxRouteMonitor::rtmSendRoute(int action, int flags, int type,
                                       const IPAddress& prefix) {
    constexpr size_t rtm_max_size = sizeof(struct rtmsg) +
                                    2 * RTA_SPACE(sizeof(uint32_t)) +
                                    RTA_SPACE(sizeof(struct in6_addr));
    wg_allowedip ip;
    if (!buildAllowedIp(&ip, prefix)) {
    logger.warning() << "Invalid destination prefix";
    return false;
    }

    char buf[NLMSG_SPACE(rtm_max_size)];
    struct nlmsghdr* nlmsg = reinterpret_cast<struct nlmsghdr*>(buf);
    struct rtmsg* rtm = static_cast<struct rtmsg*>(NLMSG_DATA(nlmsg));

    memset(buf, 0, sizeof(buf));
    nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlmsg->nlmsg_type = action;
    nlmsg->nlmsg_flags = flags;
    nlmsg->nlmsg_pid = getpid();
    nlmsg->nlmsg_seq = m_nlseq++;
    rtm->rtm_dst_len = ip.cidr;
    rtm->rtm_family = ip.family;
    rtm->rtm_type = type;
    rtm->rtm_table = RT_TABLE_UNSPEC;
    rtm->rtm_protocol = RTPROT_BOOT;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;

    if (rtm->rtm_family == AF_INET6) {
    nlmsg_append_attr(nlmsg, sizeof(buf), RTA_DST, &ip.ip6, sizeof(ip.ip6));
    } else {
    nlmsg_append_attr(nlmsg, sizeof(buf), RTA_DST, &ip.ip4, sizeof(ip.ip4));
    }

    if (rtm->rtm_type == RTN_UNICAST) {
    int index = if_nametoindex(WG_INTERFACE);

    if (index <= 0) {
        logger.error() << "if_nametoindex() failed:" << strerror(errno);
        return false;
    }
    nlmsg_append_attr32(nlmsg, sizeof(buf), RTA_OIF, index);
    }

    if (rtm->rtm_type == RTN_THROW) {
    int index = if_nametoindex(getgatewayandiface().toUtf8());
    if (index <= 0) {
        logger.error() << "if_nametoindex() failed:" << strerror(errno);
        return false;
    }
    nlmsg_append_attr32(nlmsg, sizeof(buf), RTA_OIF, index);
    }

    struct sockaddr_nl nladdr;
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    size_t result = sendto(m_nlsock, buf, nlmsg->nlmsg_len, 0,
                           (struct sockaddr*)&nladdr, sizeof(nladdr));

    return (result == nlmsg->nlmsg_len);
}

static void nlmsg_append_attr(struct nlmsghdr* nlmsg, size_t maxlen,
                              int attrtype, const void* attrdata,
                              size_t attrlen) {
    size_t newlen = NLMSG_ALIGN(nlmsg->nlmsg_len) + RTA_SPACE(attrlen);
    if (newlen <= maxlen) {
    char* buf = reinterpret_cast<char*>(nlmsg) + NLMSG_ALIGN(nlmsg->nlmsg_len);
    struct rtattr* attr = reinterpret_cast<struct rtattr*>(buf);
    attr->rta_type = attrtype;
    attr->rta_len = RTA_LENGTH(attrlen);
    memcpy(RTA_DATA(attr), attrdata, attrlen);
    nlmsg->nlmsg_len = newlen;
    }
}

static void nlmsg_append_attr32(struct nlmsghdr* nlmsg, size_t maxlen,
                                int attrtype, uint32_t value) {
    nlmsg_append_attr(nlmsg, maxlen, attrtype, &value, sizeof(value));
}

void LinuxRouteMonitor::nlsockReady() {
    char buf[1024];
    ssize_t len = recv(m_nlsock, buf, sizeof(buf), MSG_DONTWAIT);
    if (len <= 0) {
    return;
    }

    struct nlmsghdr* nlmsg = (struct nlmsghdr*)buf;
    while (NLMSG_OK(nlmsg, len)) {
    if (nlmsg->nlmsg_type == NLMSG_DONE) {
        return;
    }
    if (nlmsg->nlmsg_type != NLMSG_ERROR) {
        nlmsg = NLMSG_NEXT(nlmsg, len);
        continue;
    }
    struct nlmsgerr* err = static_cast<struct nlmsgerr*>(NLMSG_DATA(nlmsg));
    if (err->error != 0) {
        logger.debug() << "Netlink request failed:" << strerror(-err->error);
    }
    nlmsg = NLMSG_NEXT(nlmsg, len);
    }
}

#define BUFFER_SIZE 4096

QString LinuxRouteMonitor::getgatewayandiface()
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
        logger.debug() << "Gateway " << gateway_address << " for interface " << interface;
        break;
    }
    }
    close(sock);
    return interface;
}

static bool buildAllowedIp(wg_allowedip* ip,
                                         const IPAddress& prefix) {
    const char* addrString = qPrintable(prefix.address().toString());
    if (prefix.type() == QAbstractSocket::IPv4Protocol) {
    ip->family = AF_INET;
    ip->cidr = prefix.prefixLength();
    return inet_pton(AF_INET, addrString, &ip->ip4) == 1;
    }
    if (prefix.type() == QAbstractSocket::IPv6Protocol) {
    ip->family = AF_INET6;
    ip->cidr = prefix.prefixLength();
    return inet_pton(AF_INET6, addrString, &ip->ip6) == 1;
    }
    return false;
}
