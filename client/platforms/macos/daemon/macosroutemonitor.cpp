/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosroutemonitor.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QProcess>
#include <QScopeGuard>
#include <QTimer>

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("MacosRouteMonitor");
}  // namespace

MacosRouteMonitor::MacosRouteMonitor(const QString& ifname, QObject* parent)
    : QObject(parent), m_ifname(ifname) {
  MZ_COUNT_CTOR(MacosRouteMonitor);
  logger.debug() << "MacosRouteMonitor created.";

  m_rtsock = socket(PF_ROUTE, SOCK_RAW, 0);
  if (m_rtsock < 0) {
    logger.error() << "Failed to create routing socket:" << strerror(errno);
    return;
  }

  m_ifindex = if_nametoindex(qPrintable(ifname));

  m_notifier = new QSocketNotifier(m_rtsock, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &MacosRouteMonitor::rtsockReady);

  // Grab the default routes at startup.
  rtmFetchRoutes(AF_INET);
  rtmFetchRoutes(AF_INET6);
}

MacosRouteMonitor::~MacosRouteMonitor() {
  MZ_COUNT_DTOR(MacosRouteMonitor);
  flushExclusionRoutes();
  if (m_rtsock >= 0) {
    close(m_rtsock);
  }
  logger.debug() << "MacosRouteMonitor destroyed.";
}

// Compare memory against zero.
static int memcmpzero(const void* data, size_t len) {
  const quint8* ptr = static_cast<const quint8*>(data);
  while (len--) {
    if (*ptr++) return 1;
  }
  return 0;
}

void MacosRouteMonitor::handleRtmDelete(const struct rt_msghdr* rtm,
                                        const QByteArray& payload) {
  QList<QByteArray> addrlist = parseAddrList(payload);

  // Ignore routing changes on the tunnel interface.
  if (rtm->rtm_index == m_ifindex) {
    return;
  }

  QStringList list;
#ifdef MZ_DEBUG
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
#endif
  char ifname[IF_NAMESIZE] = "null";
  if (rtm->rtm_index != 0) {
    if_indextoname(rtm->rtm_index, ifname);
  }
  logger.debug() << "Route deleted via" << ifname
                 << QString("addrs(%1):").arg(rtm->rtm_addrs, 0, 16)
                 << list.join(" ");

  // We expect all useful routes to contain a destination, netmask and gateway.
  if (!(rtm->rtm_addrs & RTA_DST) || !(rtm->rtm_addrs & RTA_GATEWAY) ||
      !(rtm->rtm_addrs & RTA_NETMASK) || (addrlist.count() < 3)) {
    return;
  }

  // Check for a default route, which should have a netmask of zero.
  const struct sockaddr* sa =
      reinterpret_cast<const struct sockaddr*>(addrlist[2].constData());
  if (sa->sa_family == AF_INET) {
    struct sockaddr_in sin;
    Q_ASSERT(sa->sa_len <= sizeof(sin));
    memset(&sin, 0, sizeof(sin));
    memcpy(&sin, sa, sa->sa_len);
    if (memcmpzero(&sin.sin_addr, sizeof(sin.sin_addr)) != 0) {
      return;
    }
  } else if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 sin6;
    Q_ASSERT(sa->sa_len <= sizeof(sin6));
    memset(&sin6, 0, sizeof(sin6));
    memcpy(&sin6, sa, sa->sa_len);
    if (memcmpzero(&sin6.sin6_addr, sizeof(sin6.sin6_addr)) != 0) {
      return;
    }
  } else if (sa->sa_family != AF_UNSPEC) {
    // We have sometimes seen the default route reported with AF_UNSPEC.
    return;
  }

  // Clear the default gateway
  const struct sockaddr* dst =
      reinterpret_cast<const struct sockaddr*>(addrlist[0].constData());
  QAbstractSocket::NetworkLayerProtocol protocol;
  unsigned int plen;
  if (dst->sa_family == AF_INET) {
    m_defaultGatewayIpv4.clear();
    m_defaultIfindexIpv4 = 0;
    protocol = QAbstractSocket::IPv4Protocol;
    plen = 32;
  } else if (dst->sa_family == AF_INET6) {
    m_defaultGatewayIpv6.clear();
    m_defaultIfindexIpv6 = 0;
    protocol = QAbstractSocket::IPv6Protocol;
    plen = 128;
  }

  logger.debug() << "Lost default route via" << ifname
                 << logger.sensitive(addrToString(addrlist[1]));
  for (const QHostAddress& addr : m_exclusionRoutes) {
    if (addr.protocol() == protocol) {
      logger.debug() << "Removing exclusion route to"
                     << logger.sensitive(addr.toString());
      rtmSendRoute(RTM_DELETE, addr, plen, rtm->rtm_index, nullptr);
    }
  }
}

void MacosRouteMonitor::handleRtmUpdate(const struct rt_msghdr* rtm,
                                        const QByteArray& payload) {
  QList<QByteArray> addrlist = parseAddrList(payload);
  int ifindex = rtm->rtm_index;
  char ifname[IF_NAMESIZE] = "null";

  // We expect all useful routes to contain a destination, netmask and gateway.
  if (!(rtm->rtm_addrs & RTA_DST) || !(rtm->rtm_addrs & RTA_GATEWAY) ||
      !(rtm->rtm_addrs & RTA_NETMASK) || (addrlist.count() < 3)) {
    return;
  }
  // Ignore route changes that we caused, or routes on the tunnel interface.
  if (rtm->rtm_index == m_ifindex) {
    return;
  }
  if ((rtm->rtm_pid == getpid()) && (rtm->rtm_type != RTM_GET)) {
    return;
  }

  // Special case: If RTA_IFP is set, then we should get the interface index
  // from the address list instead of rtm_index.
  if (rtm->rtm_addrs & RTA_IFP) {
    int addridx = 0;
    for (int mask = 1; mask < RTA_IFP; mask <<= 1) {
      if (rtm->rtm_addrs & mask) {
        addridx++;
      }
    }
    if (addridx >= addrlist.count()) {
      return;
    }
    const char* sdl_data = addrlist[addridx].constData();
    const struct sockaddr_dl* sdl =
        reinterpret_cast<const struct sockaddr_dl*>(sdl_data);
    if (sdl->sdl_family == AF_LINK) {
      ifindex = sdl->sdl_index;
    }
  }

  // Log relevant updates to the routing table.
  QStringList list;
#ifdef MZ_DEBUG
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
#endif
  if_indextoname(ifindex, ifname);
  logger.debug() << "Route update via" << ifname
                 << QString("addrs(%1):").arg(rtm->rtm_addrs, 0, 16)
                 << list.join(" ");

  // Check for a default route, which should have a netmask of zero.
  const struct sockaddr* sa =
      reinterpret_cast<const struct sockaddr*>(addrlist[2].constData());
  if (sa->sa_family == AF_INET) {
    struct sockaddr_in sin;
    Q_ASSERT(sa->sa_len <= sizeof(sin));
    memset(&sin, 0, sizeof(sin));
    memcpy(&sin, sa, sa->sa_len);
    if (memcmpzero(&sin.sin_addr, sizeof(sin.sin_addr)) != 0) {
      return;
    }
  } else if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 sin6;
    Q_ASSERT(sa->sa_len <= sizeof(sin6));
    memset(&sin6, 0, sizeof(sin6));
    memcpy(&sin6, sa, sa->sa_len);
    if (memcmpzero(&sin6.sin6_addr, sizeof(sin6.sin6_addr)) != 0) {
      return;
    }
  } else if (sa->sa_family != AF_UNSPEC) {
    // The default route sometimes sets a netmask of AF_UNSPEC.
    return;
  }

  // Determine if this is the IPv4 or IPv6 default route.
  const struct sockaddr* dst =
      reinterpret_cast<const struct sockaddr*>(addrlist[0].constData());
  QAbstractSocket::NetworkLayerProtocol protocol;
  unsigned int plen;
  int rtm_type = RTM_ADD;
  if (dst->sa_family == AF_INET) {
    if (m_defaultIfindexIpv4 != 0) {
      rtm_type = RTM_CHANGE;
    }
    m_defaultGatewayIpv4 = addrlist[1];
    m_defaultIfindexIpv4 = ifindex;
    protocol = QAbstractSocket::IPv4Protocol;
    plen = 32;
  } else if (dst->sa_family == AF_INET6) {
    if (m_defaultIfindexIpv6 != 0) {
      rtm_type = RTM_CHANGE;
    }
    m_defaultGatewayIpv6 = addrlist[1];
    m_defaultIfindexIpv6 = ifindex;
    protocol = QAbstractSocket::IPv6Protocol;
    plen = 128;
  } else {
    return;
  }

  // Update the exclusion routes with the new default route.
  logger.debug() << "Updating default route via" << ifname
                 << addrToString(addrlist[1]);
  for (const QHostAddress& addr : m_exclusionRoutes) {
    if (addr.protocol() == protocol) {
      logger.debug() << "Updating exclusion route to"
                     << logger.sensitive(addr.toString());
      rtmSendRoute(rtm_type, addr, plen, ifindex, addrlist[1].constData());
    }
  }
}

void MacosRouteMonitor::handleIfaceInfo(const struct if_msghdr* ifm,
                                        const QByteArray& payload) {
  QStringList list;

  if (ifm->ifm_index != if_nametoindex(qPrintable(m_ifname))) {
    return;
  }
  m_ifflags = ifm->ifm_flags;

#ifdef MZ_DEBUG
  QList<QByteArray> addrlist = parseAddrList(payload);
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
#else
  Q_UNUSED(payload);
#endif
  logger.debug() << "Interface" << ifm->ifm_index
                 << "chagned flags:" << ifm->ifm_flags
                 << QString("addrs(%1):").arg(ifm->ifm_addrs, 0, 16)
                 << list.join(" ");
}

void MacosRouteMonitor::rtsockReady() {
  char buf[1024];
  ssize_t len = recv(m_rtsock, buf, sizeof(buf), MSG_DONTWAIT);
  if (len <= 0) {
    return;
  }

#ifndef RTMSG_NEXT
#  define RTMSG_NEXT(_rtm_) \
    (struct rt_msghdr*)((char*)(_rtm_) + (_rtm_)->rtm_msglen)
#endif

  struct rt_msghdr* rtm = reinterpret_cast<struct rt_msghdr*>(buf);
  struct rt_msghdr* end = reinterpret_cast<struct rt_msghdr*>(&buf[len]);
  while (rtm < end) {
    // Ensure the message fits within the buffer
    if (RTMSG_NEXT(rtm) > end) {
      logger.debug() << "Routing message overflowed with length"
                     << rtm->rtm_msglen;
      break;
    }

    // Handle the routing message.
    QByteArray message((char*)rtm, rtm->rtm_msglen);
    switch (rtm->rtm_type) {
      case RTM_ADD:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmUpdate(rtm, message);
        break;
      case RTM_DELETE:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmDelete(rtm, message);
        break;
      case RTM_CHANGE:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmUpdate(rtm, message);
        break;
      case RTM_GET:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmUpdate(rtm, message);
        break;
      case RTM_IFINFO:
        message.remove(0, sizeof(struct if_msghdr));
        handleIfaceInfo((struct if_msghdr*)rtm, message);
        break;
      default:
        break;
    }

    rtm = RTMSG_NEXT(rtm);
  }
}

void MacosRouteMonitor::rtmAppendAddr(struct rt_msghdr* rtm, size_t maxlen,
                                      int rtaddr, const void* sa) {
  size_t sa_len = static_cast<const struct sockaddr*>(sa)->sa_len;
  Q_ASSERT((rtm->rtm_addrs & rtaddr) == 0);
  if ((rtm->rtm_msglen + sa_len) > maxlen) {
    return;
  }

  memcpy((char*)rtm + rtm->rtm_msglen, sa, sa_len);
  rtm->rtm_addrs |= rtaddr;
  rtm->rtm_msglen += sa_len;
  if (rtm->rtm_msglen % sizeof(uint32_t)) {
    rtm->rtm_msglen += sizeof(uint32_t) - (rtm->rtm_msglen % sizeof(uint32_t));
  }
}

bool MacosRouteMonitor::rtmSendRoute(int action, const QHostAddress& prefix,
                                     unsigned int plen, unsigned int ifindex,
                                     const void* gateway) {
  constexpr size_t rtm_max_size = sizeof(struct rt_msghdr) +
                                  sizeof(struct sockaddr_in6) * 2 +
                                  sizeof(struct sockaddr_storage);
  char buf[rtm_max_size] = {0};
  struct rt_msghdr* rtm = reinterpret_cast<struct rt_msghdr*>(buf);

  rtm->rtm_msglen = sizeof(struct rt_msghdr);
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = action;
  rtm->rtm_index = ifindex;
  rtm->rtm_flags = RTF_STATIC | RTF_UP;
  rtm->rtm_addrs = 0;
  rtm->rtm_pid = 0;
  rtm->rtm_seq = m_rtseq++;
  rtm->rtm_errno = 0;
  rtm->rtm_inits = 0;
  memset(&rtm->rtm_rmx, 0, sizeof(rtm->rtm_rmx));

  // Append RTA_DST
  if (prefix.protocol() == QAbstractSocket::IPv6Protocol) {
    struct sockaddr_in6 sin6;
    Q_IPV6ADDR dst = prefix.toIPv6Address();
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len = sizeof(sin6);
    memcpy(&sin6.sin6_addr, &dst, 16);
    rtmAppendAddr(rtm, rtm_max_size, RTA_DST, &sin6);
  } else {
    struct sockaddr_in sin;
    quint32 dst = prefix.toIPv4Address();
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(sin);
    sin.sin_addr.s_addr = htonl(dst);
    rtmAppendAddr(rtm, rtm_max_size, RTA_DST, &sin);
  }

  // Append RTA_GATEWAY
  if (gateway != nullptr) {
    int family = static_cast<const struct sockaddr*>(gateway)->sa_family;
    if ((family == AF_INET) || (family == AF_INET6)) {
      rtm->rtm_flags |= RTF_GATEWAY;
    }
    rtmAppendAddr(rtm, rtm_max_size, RTA_GATEWAY, gateway);
  }

  // Append RTA_NETMASK
  if (prefix.protocol() == QAbstractSocket::IPv6Protocol) {
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len = sizeof(sin6);
    memset(&sin6.sin6_addr.s6_addr, 0xff, plen / 8);
    if (plen % 8) {
      sin6.sin6_addr.s6_addr[plen / 8] = 0xFF ^ (0xFF >> (plen % 8));
    }
    rtmAppendAddr(rtm, rtm_max_size, RTA_NETMASK, &sin6);
  } else if (prefix.protocol() == QAbstractSocket::IPv4Protocol) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(struct sockaddr_in);
    sin.sin_addr.s_addr = 0xffffffff;
    if (plen < 32) {
      sin.sin_addr.s_addr ^= htonl(0xffffffff >> plen);
    }
    rtmAppendAddr(rtm, rtm_max_size, RTA_NETMASK, &sin);
  }

  // Send the routing message to the kernel.
  int len = write(m_rtsock, rtm, rtm->rtm_msglen);
  if (len == rtm->rtm_msglen) {
    return true;
  }
  if ((action == RTM_ADD) && (errno == EEXIST)) {
    return true;
  }
  if ((action == RTM_DELETE) && (errno == ESRCH)) {
    return true;
  }
  logger.warning() << "Failed to send routing message:" << strerror(errno);
  return false;
}

bool MacosRouteMonitor::rtmFetchRoutes(int family) {
  constexpr size_t rtm_max_size =
      sizeof(struct rt_msghdr) + sizeof(struct sockaddr_storage) * 2;
  char buf[rtm_max_size] = {0};
  struct rt_msghdr* rtm = reinterpret_cast<struct rt_msghdr*>(buf);

  rtm->rtm_msglen = sizeof(struct rt_msghdr);
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = RTM_GET;
  rtm->rtm_flags = RTF_UP | RTF_GATEWAY;
  rtm->rtm_addrs = 0;
  rtm->rtm_pid = 0;
  rtm->rtm_seq = m_rtseq++;
  rtm->rtm_errno = 0;
  rtm->rtm_inits = 0;
  memset(&rtm->rtm_rmx, 0, sizeof(rtm->rtm_rmx));

  if (family == AF_INET) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(struct sockaddr_in);
    rtmAppendAddr(rtm, rtm_max_size, RTA_DST, &sin);
    rtmAppendAddr(rtm, rtm_max_size, RTA_NETMASK, &sin);
  } else if (family == AF_INET6) {
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(struct sockaddr_in6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len = sizeof(struct sockaddr_in6);
    rtmAppendAddr(rtm, rtm_max_size, RTA_DST, &sin6);
    rtmAppendAddr(rtm, rtm_max_size, RTA_NETMASK, &sin6);
  } else {
    logger.warning() << "Unsupported address family";
    return false;
  }

  // Send the routing message into the kernel.
  int len = write(m_rtsock, rtm, rtm->rtm_msglen);
  if (len == rtm->rtm_msglen) {
    return true;
  }
  logger.warning() << "Failed to request routing table:" << strerror(errno);
  return false;
}

bool MacosRouteMonitor::insertRoute(const IPAddress& prefix) {
  struct sockaddr_dl datalink;
  memset(&datalink, 0, sizeof(datalink));
  datalink.sdl_family = AF_LINK;
  datalink.sdl_len = offsetof(struct sockaddr_dl, sdl_data) + m_ifname.length();
  datalink.sdl_index = m_ifindex;
  datalink.sdl_type = IFT_OTHER;
  datalink.sdl_nlen = m_ifname.length();
  datalink.sdl_alen = 0;
  datalink.sdl_slen = 0;
  memcpy(&datalink.sdl_data, qPrintable(m_ifname), datalink.sdl_nlen);

  return rtmSendRoute(RTM_ADD, prefix.address(), prefix.prefixLength(),
                      m_ifindex, &datalink);
}

bool MacosRouteMonitor::deleteRoute(const IPAddress& prefix) {
  return rtmSendRoute(RTM_DELETE, prefix.address(), prefix.prefixLength(),
                      m_ifindex, nullptr);
}

bool MacosRouteMonitor::addExclusionRoute(const QHostAddress& address) {
  logger.debug() << "Adding exclusion route for"
                 << logger.sensitive(address.toString());

  if (m_exclusionRoutes.contains(address)) {
    logger.warning() << "Exclusion route already exists";
    return false;
  }
  m_exclusionRoutes.append(address);

  // If the default route is known, then updte the routing table immediately.
  if ((address.protocol() == QAbstractSocket::IPv4Protocol) &&
      (m_defaultIfindexIpv4 != 0) && !m_defaultGatewayIpv4.isEmpty()) {
    return rtmSendRoute(RTM_ADD, address, 32, m_defaultIfindexIpv4,
                        m_defaultGatewayIpv4.constData());
  }
  if ((address.protocol() == QAbstractSocket::IPv6Protocol) &&
      (m_defaultIfindexIpv6 != 0) && !m_defaultGatewayIpv6.isEmpty()) {
    return rtmSendRoute(RTM_ADD, address, 128, m_defaultIfindexIpv6,
                        m_defaultGatewayIpv6.constData());
  }

  // Otherwise, the default route isn't known yet. Do nothing.
  return true;
}

bool MacosRouteMonitor::deleteExclusionRoute(const QHostAddress& address) {
  logger.debug() << "Deleting exclusion route for"
                 << logger.sensitive(address.toString());

  m_exclusionRoutes.removeAll(address);
  if (address.protocol() == QAbstractSocket::IPv4Protocol) {
    return rtmSendRoute(RTM_DELETE, address, 32, m_defaultIfindexIpv4, nullptr);
  } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
    return rtmSendRoute(RTM_DELETE, address, 128, m_defaultIfindexIpv6,
                        nullptr);
  } else {
    return false;
  }
}

void MacosRouteMonitor::flushExclusionRoutes() {
  while (!m_exclusionRoutes.isEmpty()) {
    QHostAddress address = m_exclusionRoutes.takeFirst();
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
      rtmSendRoute(RTM_DELETE, address, 32, m_defaultIfindexIpv4, nullptr);
    } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
      rtmSendRoute(RTM_DELETE, address, 128, m_defaultIfindexIpv6, nullptr);
    }
  }
}

// static
QList<QByteArray> MacosRouteMonitor::parseAddrList(const QByteArray& payload) {
  QList<QByteArray> list;
  int offset = 0;
  constexpr int minlen = offsetof(struct sockaddr, sa_len) + sizeof(u_short);

  while ((offset + minlen) <= payload.length()) {
    struct sockaddr* sa = (struct sockaddr*)(payload.constData() + offset);
    int paddedSize = sa->sa_len;
    if (!paddedSize || (paddedSize % sizeof(uint32_t))) {
      paddedSize += sizeof(uint32_t) - (paddedSize % sizeof(uint32_t));
    }
    if ((offset + paddedSize) > payload.length()) {
      break;
    }
    list.append(payload.mid(offset, paddedSize));
    offset += paddedSize;
  }
  return list;
}

// static
QString MacosRouteMonitor::addrToString(const struct sockaddr* sa) {
  if (sa->sa_family == AF_INET) {
    const struct sockaddr_in* sin = (const struct sockaddr_in*)sa;
    return QString(inet_ntoa(sin->sin_addr));
  }
  if (sa->sa_family == AF_INET6) {
    const struct sockaddr_in6* sin6 = (const struct sockaddr_in6*)sa;
    char buf[INET6_ADDRSTRLEN];
    return QString(inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf)));
  }
  if (sa->sa_family == AF_LINK) {
    const struct sockaddr_dl* sdl = (const struct sockaddr_dl*)sa;
    return QString("link#%1:").arg(sdl->sdl_index) + QString(link_ntoa(sdl));
  }
  if (sa->sa_family == AF_UNSPEC) {
    return QString("unspec");
  }
  return QString("unknown(af=%1)").arg(sa->sa_family);
}

// static
QString MacosRouteMonitor::addrToString(const QByteArray& data) {
  const struct sockaddr* sa = (const struct sockaddr*)data.constData();
  Q_ASSERT(sa->sa_len <= data.length());
  return addrToString(sa);
}
