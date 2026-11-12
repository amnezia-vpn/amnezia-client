/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxroutemonitor.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QNetworkInterface>
#include <QProcess>
#include <QScopeGuard>
#include <QTimer>
#include <poll.h>

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "core/utils/utilities.h"
#include "leakdetector.h"
#include "logger.h"
#include "core/utils/networkUtilities.h"

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
}

LinuxRouteMonitor::~LinuxRouteMonitor() {
  MZ_COUNT_DTOR(LinuxRouteMonitor);
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

bool LinuxRouteMonitor::rtmSendRoute(int action, int flags, int type, const IPAddress &prefix)
{
    return routeMutationSucceeded(rtmSendRouteWithResult(action, flags, type, prefix));
}

RouteMutationResult LinuxRouteMonitor::createRoute(const IPAddress &prefix)
{
    return rtmSendRouteWithResult(RTM_NEWROUTE, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK, RTN_UNICAST,
                                  prefix);
}

RouteCreationResult LinuxRouteMonitor::createExclusionRoute(const IPAddress &prefix)
{
    const auto result = rtmSendRouteWithResult(RTM_NEWROUTE, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK,
                                               RTN_THROW, prefix);
    if (result == RouteMutationResult::Applied) {
        return { SplitTunnelingUpdateResult::Applied, { prefix } };
    }
    return { result == RouteMutationResult::StateUnknown ? SplitTunnelingUpdateResult::StateUnknown
                                                         : SplitTunnelingUpdateResult::NotApplied,
             { } };
}

RouteMutationResult LinuxRouteMonitor::rtmSendRouteWithResult(int action, int flags, int type, const IPAddress &prefix)
{
    constexpr size_t rtm_max_size =
            sizeof(struct rtmsg) + 2 * RTA_SPACE(sizeof(uint32_t)) + RTA_SPACE(sizeof(struct in6_addr));
    wg_allowedip ip;
    if (!buildAllowedIp(&ip, prefix)) {
        logger.warning() << "Invalid destination prefix";
        return RouteMutationResult::NotApplied;
    }

    char buf[NLMSG_SPACE(rtm_max_size)];
    struct nlmsghdr *nlmsg = reinterpret_cast<struct nlmsghdr *>(buf);
    struct rtmsg *rtm = static_cast<struct rtmsg *>(NLMSG_DATA(nlmsg));

    memset(buf, 0, sizeof(buf));
    nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlmsg->nlmsg_type = action;
    nlmsg->nlmsg_flags = flags;
    nlmsg->nlmsg_pid = 0;
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
        int index = if_nametoindex(qPrintable(m_ifname));

        if (index <= 0) {
            logger.error() << "if_nametoindex() failed:" << strerror(errno);
            return RouteMutationResult::NotApplied;
        }
        nlmsg_append_attr32(nlmsg, sizeof(buf), RTA_OIF, index);
        nlmsg_append_attr32(nlmsg, sizeof(buf), RTA_PRIORITY, 1);
    }

    const bool exclusion = rtm->rtm_type == RTN_THROW;
    QString gateway;
    if (exclusion) {
        if (action == RTM_DELROUTE && !m_exclusionGateways.contains(prefix))
            return RouteMutationResult::Unchanged;
        gateway = action == RTM_DELROUTE ? m_exclusionGateways.value(prefix)
                                         : NetworkUtilities::getGatewayAndIface().first;
        if (gateway.isEmpty()) {
            logger.warning() << "No default gateway available, skipping exclusion route";
            return RouteMutationResult::NotApplied;
        }
        struct in_addr ip4;
        if (inet_pton(AF_INET, gateway.toUtf8().constData(), &ip4) != 1)
            return RouteMutationResult::NotApplied;
        nlmsg_append_attr(nlmsg, sizeof(buf), RTA_GATEWAY, &ip4, sizeof(ip4));
        nlmsg_append_attr32(nlmsg, sizeof(buf), RTA_PRIORITY, 0);
        rtm->rtm_type = RTN_UNICAST;
    }

    const int socketFd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
    if (socketFd < 0) {
        return RouteMutationResult::NotApplied;
    }
    const auto closeSocket = qScopeGuard([&] { close(socketFd); });
    struct sockaddr_nl kernel { };
    kernel.nl_family = AF_NETLINK;
    const auto sent =
            sendto(socketFd, buf, nlmsg->nlmsg_len, 0, reinterpret_cast<struct sockaddr *>(&kernel), sizeof(kernel));
    if (sent < 0) {
        return RouteMutationResult::NotApplied;
    }
    if (sent != nlmsg->nlmsg_len) {
        return RouteMutationResult::StateUnknown;
    }
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < 1000) {
        struct pollfd descriptor { socketFd, POLLIN, 0 };
        const int ready = poll(&descriptor, 1, 1000 - elapsed.elapsed());
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready <= 0 || !(descriptor.revents & POLLIN)) {
            return RouteMutationResult::StateUnknown;
        }
        char reply[8192];
        struct sockaddr_nl sender { };
        socklen_t senderLength = sizeof(sender);
        auto length = recvfrom(socketFd, reply, sizeof(reply), MSG_DONTWAIT,
                               reinterpret_cast<struct sockaddr *>(&sender), &senderLength);
        if (length < 0 && (errno == EINTR || errno == EAGAIN)) {
            continue;
        }
        if (length <= 0 || sender.nl_pid != 0) {
            return RouteMutationResult::StateUnknown;
        }
        auto *message = reinterpret_cast<struct nlmsghdr *>(reply);
        for (; NLMSG_OK(message, length); message = NLMSG_NEXT(message, length)) {
            if (message->nlmsg_seq != nlmsg->nlmsg_seq) {
                continue;
            }
            if (message->nlmsg_type != NLMSG_ERROR || message->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
                return RouteMutationResult::StateUnknown;
            }
            const auto *error = static_cast<const struct nlmsgerr *>(NLMSG_DATA(message));
            if (error->error == 0) {
                if (exclusion) {
                    if (action == RTM_NEWROUTE)
                        m_exclusionGateways.insert(prefix, gateway);
                    else
                        m_exclusionGateways.remove(prefix);
                }
                return RouteMutationResult::Applied;
            }
            if (action == RTM_NEWROUTE && error->error == -EEXIST) {
                return RouteMutationResult::Unchanged;
            }
            if (action == RTM_DELROUTE && (error->error == -ESRCH || error->error == -ENOENT)) {
                if (exclusion)
                    m_exclusionGateways.remove(prefix);
                return RouteMutationResult::Unchanged;
            }
            logger.warning() << "Route request rejected:" << strerror(-error->error);
            return RouteMutationResult::NotApplied;
        }
    }
    return RouteMutationResult::StateUnknown;
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

static bool buildAllowedIp(wg_allowedip *ip, const IPAddress &prefix)
{
    const QByteArray address = prefix.address().toString().toUtf8();
    const char *addrString = address.constData();
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
