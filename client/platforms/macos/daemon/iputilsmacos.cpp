/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iputilsmacos.h"
#include "leakdetector.h"
#include "logger.h"
#include "macosdaemon.h"
#include "daemon/wireguardutils.h"

#include <QHostAddress>
#include <QScopeGuard>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_var.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <sys/ioctl.h>
#include <unistd.h>

constexpr uint32_t ETH_MTU = 1500;
constexpr uint32_t WG_MTU_OVERHEAD = 80;

namespace {
Logger logger(LOG_MACOS, "IPUtilsMacos");
}

IPUtilsMacos::IPUtilsMacos(QObject* parent) : IPUtils(parent) {
  MVPN_COUNT_CTOR(IPUtilsMacos);
  logger.debug() << "IPUtilsMacos created.";
}

IPUtilsMacos::~IPUtilsMacos() {
  MVPN_COUNT_DTOR(IPUtilsMacos);
  logger.debug() << "IPUtilsMacos destroyed.";
}

bool IPUtilsMacos::addInterfaceIPs(const InterfaceConfig& config) {
  if (!addIP4AddressToDevice(config)) {
    return false;
  }
  if (config.m_ipv6Enabled) {
    if (!addIP6AddressToDevice(config)) {
      return false;
    }
  }
  return true;
}

bool IPUtilsMacos::setMTUAndUp(const InterfaceConfig& config) {
  Q_UNUSED(config);
  QString ifname = MacOSDaemon::instance()->m_wgutils->interfaceName();
  struct ifreq ifr;

  // Create socket file descriptor to perform the ioctl operations on
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sockfd < 0) {
    logger.error() << "Failed to create ioctl socket.";
    return false;
  }
  auto guard = qScopeGuard([&] { close(sockfd); });

  // MTU
  strncpy(ifr.ifr_name, qPrintable(ifname), IFNAMSIZ);
  ifr.ifr_mtu = ETH_MTU - WG_MTU_OVERHEAD;
  int ret = ioctl(sockfd, SIOCSIFMTU, &ifr);
  if (ret) {
    logger.error() << "Failed to set MTU:" << strerror(errno);
    return false;
  }

  // Get the interface flags
  strncpy(ifr.ifr_name, qPrintable(ifname), IFNAMSIZ);
  ret = ioctl(sockfd, SIOCGIFFLAGS, &ifr);
  if (ret) {
    logger.error() << "Failed to get interface flags:" << strerror(errno);
    return false;
  }

  // Up
  ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
  ret = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
  if (ret) {
    logger.error() << "Failed to set device up:" << strerror(errno);
    return false;
  }

  return true;
}

bool IPUtilsMacos::addIP4AddressToDevice(const InterfaceConfig& config) {
  Q_UNUSED(config);
  QString ifname = MacOSDaemon::instance()->m_wgutils->interfaceName();
  struct ifaliasreq ifr;
  struct sockaddr_in* ifrAddr = (struct sockaddr_in*)&ifr.ifra_addr;
  struct sockaddr_in* ifrMask = (struct sockaddr_in*)&ifr.ifra_mask;
  struct sockaddr_in* ifrBcast = (struct sockaddr_in*)&ifr.ifra_broadaddr;

  // Name the interface and set family
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifra_name, qPrintable(ifname), IFNAMSIZ);

  // Get the device address to add to interface
  QPair<QHostAddress, int> parsedAddr =
      QHostAddress::parseSubnet(config.m_deviceIpv4Address);
  QByteArray _deviceAddr = parsedAddr.first.toString().toLocal8Bit();
  char* deviceAddr = _deviceAddr.data();
  ifrAddr->sin_family = AF_INET;
  ifrAddr->sin_len = sizeof(struct sockaddr_in);
  inet_pton(AF_INET, deviceAddr, &ifrAddr->sin_addr);

  // Set the netmask to /32
  ifrMask->sin_family = AF_INET;
  ifrMask->sin_len = sizeof(struct sockaddr_in);
  memset(&ifrMask->sin_addr, 0xff, sizeof(ifrMask->sin_addr));

  // Set the broadcast address.
  ifrBcast->sin_family = AF_INET;
  ifrBcast->sin_len = sizeof(struct sockaddr_in);
  ifrBcast->sin_addr.s_addr =
      (ifrAddr->sin_addr.s_addr | ~ifrMask->sin_addr.s_addr);

  // Create an IPv4 socket to perform the ioctl operations on
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sockfd < 0) {
    logger.error() << "Failed to create ioctl socket.";
    return false;
  }
  auto guard = qScopeGuard([&] { close(sockfd); });

  // Set ifr to interface
  int ret = ioctl(sockfd, SIOCAIFADDR, &ifr);
  if (ret) {
    logger.error() << "Failed to set IPv4: " << deviceAddr
                   << "error:" << strerror(errno);
    return false;
  }
  return true;
}

bool IPUtilsMacos::addIP6AddressToDevice(const InterfaceConfig& config) {
  Q_UNUSED(config);
  QString ifname = MacOSDaemon::instance()->m_wgutils->interfaceName();
  struct in6_aliasreq ifr6;

  // Name the interface and set family
  memset(&ifr6, 0, sizeof(ifr6));
  strncpy(ifr6.ifra_name, qPrintable(ifname), IFNAMSIZ);
  ifr6.ifra_addr.sin6_family = AF_INET6;
  ifr6.ifra_addr.sin6_len = sizeof(ifr6.ifra_addr);
  ifr6.ifra_lifetime.ia6t_vltime = ifr6.ifra_lifetime.ia6t_pltime = 0xffffffff;
  ifr6.ifra_prefixmask.sin6_family = AF_INET6;
  ifr6.ifra_prefixmask.sin6_len = sizeof(ifr6.ifra_prefixmask);
  memset(&ifr6.ifra_prefixmask.sin6_addr, 0xff, sizeof(struct in6_addr));

  // Get the device address to add to interface
  QPair<QHostAddress, int> parsedAddr =
      QHostAddress::parseSubnet(config.m_deviceIpv6Address);
  QByteArray _deviceAddr = parsedAddr.first.toString().toLocal8Bit();
  char* deviceAddr = _deviceAddr.data();
  inet_pton(AF_INET6, deviceAddr, &ifr6.ifra_addr.sin6_addr);

  // Create IPv4 socket to perform the ioctl operations on
  int sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP);
  if (sockfd < 0) {
    logger.error() << "Failed to create ioctl socket.";
    return false;
  }
  auto guard = qScopeGuard([&] { close(sockfd); });

  // Set ifr to interface
  int ret = ioctl(sockfd, SIOCAIFADDR_IN6, &ifr6);
  if (ret) {
    logger.error() << "Failed to set IPv6: " << deviceAddr
                   << "error:" << strerror(errno);
    return false;
  }
  return true;
}
