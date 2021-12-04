/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosroutemonitor.h"
#include "leakdetector.h"
#include "logger.h"

#include <QCoreApplication>

#include <QHostAddress>
#include <QScopeGuard>
#include <QTimer>
#include <QProcess>

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

namespace {
Logger logger(LOG_MACOS, "MacosRouteMonitor");

template <typename T>
static T* sockaddr_cast(QByteArray& data) {
  const struct sockaddr* sa = (const struct sockaddr*)data.constData();
  Q_ASSERT(sa->sa_len <= data.length());
  if (data.length() >= (int)sizeof(T)) {
    return (T*)data.data();
  }
  return nullptr;
}

}  // namespace

MacosRouteMonitor::MacosRouteMonitor(const QString& ifname, QObject* parent)
    : QObject(parent), m_ifname(ifname) {
  MVPN_COUNT_CTOR(MacosRouteMonitor);
  logger.debug() << "MacosRouteMonitor created.";

  m_rtsock = socket(PF_ROUTE, SOCK_RAW, 0);
  if (m_rtsock < 0) {
    logger.error() << "Failed to create routing socket:" << strerror(errno);
    return;
  }

  // Disable replies to our own messages.
  int off = 0;
  setsockopt(m_rtsock, SOL_SOCKET, SO_USELOOPBACK, &off, sizeof(off));

  m_notifier = new QSocketNotifier(m_rtsock, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &MacosRouteMonitor::rtsockReady);
}

MacosRouteMonitor::~MacosRouteMonitor() {
  MVPN_COUNT_DTOR(MacosRouteMonitor);
  if (m_rtsock >= 0) {
    close(m_rtsock);
  }
  logger.debug() << "MacosRouteMonitor destroyed.";
}

void MacosRouteMonitor::handleRtmAdd(const struct rt_msghdr* rtm,
                                     const QByteArray& payload) {
  QList<QByteArray> addrlist = parseAddrList(payload);

  // Ignore routing changes on the tunnel interfaces
  if ((rtm->rtm_addrs & RTA_DST) && (rtm->rtm_addrs & RTA_GATEWAY)) {
    if (m_ifname == addrToString(addrlist[1])) {
      return;
    }
  }

  QStringList list;
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
  logger.debug() << "Route added by" << rtm->rtm_pid
                 << QString("addrs(%1):").arg(rtm->rtm_addrs) << list.join(" ");
}

void MacosRouteMonitor::handleRtmDelete(const struct rt_msghdr* rtm,
                                        const QByteArray& payload) {
  QList<QByteArray> addrlist = parseAddrList(payload);

  // Ignore routing changes on the tunnel interfaces
  if ((rtm->rtm_addrs & RTA_DST) && (rtm->rtm_addrs & RTA_GATEWAY)) {
    if (m_ifname == addrToString(addrlist[1])) {
      return;
    }
  }

  QStringList list;
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
  logger.debug() << "Route deleted by" << rtm->rtm_pid
                 << QString("addrs(%1):").arg(rtm->rtm_addrs) << list.join(" ");
}

void MacosRouteMonitor::handleRtmChange(const struct rt_msghdr* rtm,
                                        const QByteArray& payload) {
  QList<QByteArray> addrlist = parseAddrList(payload);

  // Ignore routing changes on the tunnel interfaces
  if ((rtm->rtm_addrs & RTA_DST) && (rtm->rtm_addrs & RTA_GATEWAY)) {
    if (m_ifname == addrToString(addrlist[1])) {
      return;
    }
  }

  QStringList list;
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
  logger.debug() << "Route chagned by" << rtm->rtm_pid
                 << QString("addrs(%1):").arg(rtm->rtm_addrs) << list.join(" ");
}

void MacosRouteMonitor::handleIfaceInfo(const struct if_msghdr* ifm,
                                        const QByteArray& payload) {
  QList<QByteArray> addrlist = parseAddrList(payload);

  if (ifm->ifm_index != if_nametoindex(qPrintable(m_ifname))) {
    return;
  }
  m_ifflags = ifm->ifm_flags;

  QStringList list;
  for (auto addr : addrlist) {
    list.append(addrToString(addr));
  }
  logger.debug() << "Interface " << ifm->ifm_index
                 << "chagned flags:" << ifm->ifm_flags
                 << QString("addrs(%1):").arg(ifm->ifm_addrs) << list.join(" ");
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

  struct rt_msghdr* rtm = (struct rt_msghdr*)buf;
  struct rt_msghdr* end = (struct rt_msghdr*)(&buf[len]);
  while (rtm < end) {
    // Ensure the message fits within the buffer
    if (RTMSG_NEXT(rtm) > end) {
      break;
    }

    // Handle the routing message.
    QByteArray message((char*)rtm, rtm->rtm_msglen);
    switch (rtm->rtm_type) {
      case RTM_ADD:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmAdd(rtm, message);
        break;
      case RTM_DELETE:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmDelete(rtm, message);
        break;
      case RTM_CHANGE:
        message.remove(0, sizeof(struct rt_msghdr));
        handleRtmChange(rtm, message);
        break;
      case RTM_IFINFO:
        message.remove(0, sizeof(struct if_msghdr));
        handleIfaceInfo((struct if_msghdr*)rtm, message);
        break;
      default:
        logger.debug() << "Unknown routing message:" << rtm->rtm_type;
        break;
    }

    rtm = RTMSG_NEXT(rtm);
  }
}

void MacosRouteMonitor::rtmAppendAddr(struct rt_msghdr* rtm, size_t maxlen,
                                      int rtaddr, const void* sa) {
  size_t sa_len = ((struct sockaddr*)sa)->sa_len;
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

bool MacosRouteMonitor::rtmSendRoute(int action, const IPAddressRange& prefix) {
  constexpr size_t rtm_max_size = sizeof(struct rt_msghdr) +
                                  sizeof(struct sockaddr_in6) * 2 +
                                  sizeof(struct sockaddr_dl);
  char buf[rtm_max_size];
  struct rt_msghdr* rtm = (struct rt_msghdr*)buf;

  memset(rtm, 0, rtm_max_size);
  rtm->rtm_msglen = sizeof(struct rt_msghdr);
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = action;
  rtm->rtm_index = if_nametoindex(qPrintable(m_ifname));
  rtm->rtm_flags = RTF_STATIC | RTF_UP;
  rtm->rtm_addrs = 0;
  rtm->rtm_pid = 0;
  rtm->rtm_seq = m_rtseq++;
  rtm->rtm_errno = 0;
  rtm->rtm_inits = 0;
  memset(&rtm->rtm_rmx, 0, sizeof(rtm->rtm_rmx));

  // Append RTA_DST
  if (prefix.type() == IPAddressRange::IPv6) {
    struct sockaddr_in6 sin6;
    Q_IPV6ADDR dst = QHostAddress(prefix.ipAddress()).toIPv6Address();
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len = sizeof(sin6);
    memcpy(&sin6.sin6_addr, &dst, 16);
    rtmAppendAddr(rtm, rtm_max_size, RTA_DST, &sin6);
  } else {
    struct sockaddr_in sin;
    quint32 dst = QHostAddress(prefix.ipAddress()).toIPv4Address();
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(sin);
    sin.sin_addr.s_addr = htonl(dst);
    rtmAppendAddr(rtm, rtm_max_size, RTA_DST, &sin);
  }

  // Append RTA_GATEWAY
  if (action != RTM_DELETE) {
    struct sockaddr_dl sdl;
    memset(&sdl, 0, sizeof(sdl));
    sdl.sdl_family = AF_LINK;
    sdl.sdl_len = offsetof(struct sockaddr_dl, sdl_data) + m_ifname.length();
    sdl.sdl_index = rtm->rtm_index;
    sdl.sdl_type = IFT_OTHER;
    sdl.sdl_nlen = m_ifname.length();
    sdl.sdl_alen = 0;
    sdl.sdl_slen = 0;
    memcpy(&sdl.sdl_data, qPrintable(m_ifname), sdl.sdl_nlen);
    rtmAppendAddr(rtm, rtm_max_size, RTA_GATEWAY, &sdl);
  }

  // Append RTA_NETMASK
  int plen = prefix.range();
  if (prefix.type() == IPAddressRange::IPv6) {
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len = sizeof(sin6);
    memset(&sin6.sin6_addr.s6_addr, 0xff, plen / 8);
    if (plen % 8) {
      sin6.sin6_addr.s6_addr[plen / 8] = 0xFF ^ (0xFF >> (plen % 8));
    }
    rtmAppendAddr(rtm, rtm_max_size, RTA_NETMASK, &sin6);
  } else if (prefix.type() == IPAddressRange::IPv4) {
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

bool MacosRouteMonitor::insertRoute(const IPAddressRange& prefix) {
  return rtmSendRoute(RTM_ADD, prefix);
}

bool MacosRouteMonitor::deleteRoute(const IPAddressRange& prefix) {
  return rtmSendRoute(RTM_DELETE, prefix);
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
    return QString(link_ntoa(sdl));
  }
  return QString("unknown(af=%1)").arg(sa->sa_family);
}

// static
QString MacosRouteMonitor::addrToString(const QByteArray& data) {
  const struct sockaddr* sa = (const struct sockaddr*)data.constData();
  Q_ASSERT(sa->sa_len <= data.length());
  return addrToString(sa);
}
