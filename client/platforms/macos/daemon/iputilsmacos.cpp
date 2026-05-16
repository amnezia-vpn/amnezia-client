/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iputilsmacos.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_var.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

#include <QAbstractSocket>
#include <QHostAddress>
#include <QScopeGuard>

#include "daemon/wireguardutils.h"
#include "ipaddress.h"
#include "leakdetector.h"
#include "logger.h"
#include "macosdaemon.h"

namespace {
Logger logger("IPUtilsMacos");

QPair<QHostAddress, int> parseInterfaceAddress(const QString &value, int defaultPrefixLength) {
  if (value.contains("/")) {
    return QHostAddress::parseSubnet(value);
  }
  return { QHostAddress(value), defaultPrefixLength };
}
}

IPUtilsMacos::IPUtilsMacos(QObject* parent) : IPUtils(parent) {
  MZ_COUNT_CTOR(IPUtilsMacos);
  logger.debug() << "IPUtilsMacos created.";
}

IPUtilsMacos::~IPUtilsMacos() {
  MZ_COUNT_DTOR(IPUtilsMacos);
  logger.debug() << "IPUtilsMacos destroyed.";
}

bool IPUtilsMacos::addInterfaceIPs(const InterfaceConfig& config) {
  bool ret = true;
  if (!config.m_deviceIpv4Address.isEmpty()) {
    ret = addIP4AddressToDevice(config) && ret;
  }
  if (!config.m_deviceIpv6Address.isEmpty()) {
    ret = addIP6AddressToDevice(config) && ret;
  }
  return ret;
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
  ifr.ifr_mtu = config.m_deviceMTU;
  int ret = ioctl(sockfd, SIOCSIFMTU, &ifr);
  if (ret) {
    logger.error() << "Failed to set MTU -- " << config.m_deviceMTU << " -- Return code: " << ret;
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
      parseInterfaceAddress(config.m_deviceIpv4Address, 32);
  if (parsedAddr.first.protocol() != QAbstractSocket::IPv4Protocol) {
    logger.error() << "Invalid IPv4 device address:" << config.m_deviceIpv4Address;
    return false;
  }
  QByteArray _deviceAddr = parsedAddr.first.toString().toLocal8Bit();
  char* deviceAddr = _deviceAddr.data();
  ifrAddr->sin_family = AF_INET;
  ifrAddr->sin_len = sizeof(struct sockaddr_in);
  inet_pton(AF_INET, deviceAddr, &ifrAddr->sin_addr);

  const IPAddress interfaceAddress(parsedAddr.first, parsedAddr.second >= 0 ? parsedAddr.second : 32);
  QByteArray _deviceMask = interfaceAddress.netmask().toString().toLocal8Bit();
  char* deviceMask = _deviceMask.data();
  ifrMask->sin_family = AF_INET;
  ifrMask->sin_len = sizeof(struct sockaddr_in);
  inet_pton(AF_INET, deviceMask, &ifrMask->sin_addr);

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
  // Get the device address to add to interface
  QPair<QHostAddress, int> parsedAddr =
      parseInterfaceAddress(config.m_deviceIpv6Address, 128);
  if (parsedAddr.first.protocol() != QAbstractSocket::IPv6Protocol) {
    logger.error() << "Invalid IPv6 device address:" << config.m_deviceIpv6Address;
    return false;
  }
  const IPAddress interfaceAddress(parsedAddr.first, parsedAddr.second >= 0 ? parsedAddr.second : 128);
  const Q_IPV6ADDR rawPrefixMask = interfaceAddress.netmask().toIPv6Address();
  memcpy(&ifr6.ifra_prefixmask.sin6_addr, &rawPrefixMask, sizeof(rawPrefixMask));
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
