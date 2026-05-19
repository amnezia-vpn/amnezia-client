/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iputilslinux.h"

#include <arpa/inet.h>
#include <linux/if_addr.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QHostAddress>
#include <QScopeGuard>

#include "daemon/wireguardutils.h"
#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("IPUtilsLinux");
}

IPUtilsLinux::IPUtilsLinux(QObject* parent) : IPUtils(parent) {
  MZ_COUNT_CTOR(IPUtilsLinux);
  logger.debug() << "IPUtilsLinux created.";
}

IPUtilsLinux::~IPUtilsLinux() {
  MZ_COUNT_DTOR(IPUtilsLinux);
  logger.debug() << "IPUtilsLinux destroyed.";
}

bool IPUtilsLinux::addInterfaceIPs(const InterfaceConfig& config) {
  bool ret = addIP4AddressToDevice(config);
  addIP6AddressToDevice(config);
  return ret;
}

bool IPUtilsLinux::setMTUAndUp(const InterfaceConfig& config) {
  // Create socket file descriptor to perform the ioctl operations on
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sockfd < 0) {
    logger.error() << "Failed to create ioctl socket.";
    return false;
  }
  auto guard = qScopeGuard([&] { close(sockfd); });

  // Setup the interface to interact with
  struct ifreq ifr;
  strncpy(ifr.ifr_name, WG_INTERFACE, IFNAMSIZ);

  // MTU
  // FIXME: We need to know how many layers deep this particular
  // interface is into a tunnel to work effectively. Otherwise
  // we will run into fragmentation issues.
  ifr.ifr_mtu = config.m_deviceMTU;
  int ret = ioctl(sockfd, SIOCSIFMTU, &ifr);
  if (ret) {
    logger.error() << "Failed to set MTU -- " << config.m_deviceMTU << " -- Return code: " << ret;
    return false;
  }

  // Up
  ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
  ret = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
  if (ret) {
    logger.error() << "Failed to set device up -- Return code: " << ret;
    return false;
  }

  return true;
}

static bool addIPv4AddressNetlink(int ifindex, const QHostAddress& addr,
                                   int prefixlen) {
  int nlsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (nlsock < 0) return false;
  auto guard = qScopeGuard([&] { close(nlsock); });

  char buf[512];
  memset(buf, 0, sizeof(buf));

  struct nlmsghdr* nlmsg = reinterpret_cast<struct nlmsghdr*>(buf);
  nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  nlmsg->nlmsg_type = RTM_NEWADDR;
  nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
  nlmsg->nlmsg_seq = 1;
  nlmsg->nlmsg_pid = 0;

  struct ifaddrmsg* ifa = static_cast<struct ifaddrmsg*>(NLMSG_DATA(nlmsg));
  ifa->ifa_family = AF_INET;
  ifa->ifa_prefixlen = prefixlen;
  ifa->ifa_flags = IFA_F_PERMANENT;
  ifa->ifa_scope = RT_SCOPE_UNIVERSE;
  ifa->ifa_index = ifindex;

  struct in_addr ip4;
  QByteArray addrBytes = addr.toString().toLocal8Bit();
  inet_pton(AF_INET, addrBytes.constData(), &ip4);

  auto appendAttr = [](struct nlmsghdr* nlmsg, size_t maxlen, int type,
                        const void* data, size_t len) {
    size_t newlen = NLMSG_ALIGN(nlmsg->nlmsg_len) + RTA_SPACE(len);
    if (newlen > maxlen) return;
    char* p = reinterpret_cast<char*>(nlmsg) + NLMSG_ALIGN(nlmsg->nlmsg_len);
    struct rtattr* rta = reinterpret_cast<struct rtattr*>(p);
    rta->rta_type = type;
    rta->rta_len = RTA_LENGTH(len);
    memcpy(RTA_DATA(rta), data, len);
    nlmsg->nlmsg_len = newlen;
  };

  appendAttr(nlmsg, sizeof(buf), IFA_LOCAL, &ip4, sizeof(ip4));
  appendAttr(nlmsg, sizeof(buf), IFA_ADDRESS, &ip4, sizeof(ip4));

  struct sockaddr_nl nladdr;
  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;

  if (sendto(nlsock, buf, nlmsg->nlmsg_len, 0,
             reinterpret_cast<struct sockaddr*>(&nladdr),
             sizeof(nladdr)) < 0) {
    return false;
  }

  char ackbuf[1024];
  ssize_t acklen = recv(nlsock, ackbuf, sizeof(ackbuf), 0);
  if (acklen >= static_cast<ssize_t>(sizeof(struct nlmsghdr))) {
    struct nlmsghdr* ackmsg = reinterpret_cast<struct nlmsghdr*>(ackbuf);
    if (ackmsg->nlmsg_type == NLMSG_ERROR) {
      struct nlmsgerr* err = static_cast<struct nlmsgerr*>(NLMSG_DATA(ackmsg));
      if (err->error != 0) {
        errno = -err->error;
        return false;
      }
    }
  }
  return true;
}

bool IPUtilsLinux::addIP4AddressToDevice(const InterfaceConfig& config) {
  if (config.m_deviceIpv4Address.isEmpty()) return true;

  int ifindex = if_nametoindex(WG_INTERFACE);
  if (ifindex == 0) {
    logger.error() << "Failed to get ifindex for" << WG_INTERFACE;
    return false;
  }

  bool ok = false;
  const QStringList addresses =
      config.m_deviceIpv4Address.split(',', Qt::SkipEmptyParts);
  for (const QString& entry : addresses) {
    QPair<QHostAddress, int> parsed =
        QHostAddress::parseSubnet(entry.trimmed());
    if (parsed.first.isNull()) {
      logger.warning() << "Failed to parse IPv4 address:" << entry.trimmed();
      continue;
    }
    if (!addIPv4AddressNetlink(ifindex, parsed.first, parsed.second)) {
      logger.error() << "Failed to add IPv4" << parsed.first.toString() << "/"
                     << parsed.second << ":" << strerror(errno);
    } else {
      logger.debug() << "Added IPv4" << parsed.first.toString() << "/"
                     << parsed.second << "to" << WG_INTERFACE;
      ok = true;
    }
  }
  return ok;
}

bool IPUtilsLinux::addIP6AddressToDevice(const InterfaceConfig& config) {
  // Set up the ifr and the companion ifr6
  struct in6_ifreq ifr6;
  ifr6.prefixlen = 64;

  // Get the device address to add to ifr6 interface
  QPair<QHostAddress, int> parsedAddr =
      QHostAddress::parseSubnet(config.m_deviceIpv6Address);
  QByteArray _deviceAddr = parsedAddr.first.toString().toLocal8Bit();
  char* deviceAddr = _deviceAddr.data();
  inet_pton(AF_INET6, deviceAddr, &ifr6.addr);

  // Create IPv6 socket to perform the ioctl operations on
  int sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
  if (sockfd < 0) {
    logger.error() << "Failed to create ioctl socket.";
    return false;
  }
  auto guard = qScopeGuard([&] { close(sockfd); });

  // Get the index of named ifr and link with ifr6
  struct ifreq ifr;
  strncpy(ifr.ifr_name, WG_INTERFACE, IFNAMSIZ);
  ifr.ifr_addr.sa_family = AF_INET6;
  int ret = ioctl(sockfd, SIOGIFINDEX, &ifr);
  if (ret) {
    logger.error() << "Failed to get ifindex. Return code: " << ret;
    return false;
  }
  ifr6.ifindex = ifr.ifr_ifindex;

  // Set ifr6 to the interface
  ret = ioctl(sockfd, SIOCSIFADDR, &ifr6);
  if (ret && (errno != EEXIST)) {
    logger.error() << "Failed to set IPv6: " << deviceAddr
                   << "error:" << strerror(errno);
    return false;
  }

  return true;
}
