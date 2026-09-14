/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iputilslinux.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QHostAddress>
#include <QScopeGuard>

#include "daemon/wireguardutils.h"
#include "ipv4interfaceaddress.h"
#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("IPUtilsLinux");

bool addIPv4Address(int interfaceIndex, const QHostAddress& address,
                    int prefixLength) {
  const QByteArray addressBytes = address.toString().toLocal8Bit();
  struct in_addr ipv4Address = {};
  if (inet_pton(AF_INET, addressBytes.constData(), &ipv4Address) != 1) {
    errno = EINVAL;
    return false;
  }

  int socketFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (socketFd < 0) return false;
  auto guard = qScopeGuard([&] { close(socketFd); });

  char buffer[512] = {};
  auto* message = reinterpret_cast<struct nlmsghdr*>(buffer);
  message->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  message->nlmsg_type = RTM_NEWADDR;
  message->nlmsg_flags =
      NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
  message->nlmsg_seq = 1;

  auto* interfaceAddress =
      reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(message));
  interfaceAddress->ifa_family = AF_INET;
  interfaceAddress->ifa_prefixlen = prefixLength;
  interfaceAddress->ifa_scope = RT_SCOPE_UNIVERSE;
  interfaceAddress->ifa_index = interfaceIndex;

  const auto appendIPv4Attribute = [&](int type) {
    constexpr size_t payloadSize = sizeof(struct in_addr);
    const size_t attributeOffset = NLMSG_ALIGN(message->nlmsg_len);
    const size_t attributeSize = RTA_SPACE(payloadSize);
    if (attributeOffset > sizeof(buffer) ||
        attributeSize > sizeof(buffer) - attributeOffset) {
      return false;
    }

    auto* attribute = reinterpret_cast<struct rtattr*>(
        buffer + attributeOffset);
    attribute->rta_type = type;
    attribute->rta_len = RTA_LENGTH(payloadSize);

    const auto* source = reinterpret_cast<const char*>(&ipv4Address);
    auto* destination = reinterpret_cast<char*>(RTA_DATA(attribute));
    std::copy_n(source, payloadSize, destination);

    message->nlmsg_len = attributeOffset + attributeSize;
    return true;
  };

  // IFA_LOCAL keeps the configured host address; IFA_ADDRESS describes the
  // same address for a point-to-point WireGuard interface.
  if (!appendIPv4Attribute(IFA_LOCAL) ||
      !appendIPv4Attribute(IFA_ADDRESS)) {
    errno = EMSGSIZE;
    return false;
  }

  struct sockaddr_nl kernelAddress = {};
  kernelAddress.nl_family = AF_NETLINK;
  if (sendto(socketFd, buffer, message->nlmsg_len, 0,
             reinterpret_cast<struct sockaddr*>(&kernelAddress),
             sizeof(kernelAddress)) < 0) {
    return false;
  }

  char acknowledgementBuffer[1024] = {};
  const ssize_t acknowledgementLength =
      recv(socketFd, acknowledgementBuffer, sizeof(acknowledgementBuffer), 0);
  if (acknowledgementLength < static_cast<ssize_t>(sizeof(struct nlmsghdr))) {
    return acknowledgementLength >= 0;
  }

  auto* acknowledgement =
      reinterpret_cast<struct nlmsghdr*>(acknowledgementBuffer);
  if (acknowledgement->nlmsg_type != NLMSG_ERROR) return true;

  auto* error = reinterpret_cast<struct nlmsgerr*>(NLMSG_DATA(acknowledgement));
  if (error->error == 0) return true;

  errno = -error->error;
  return false;
}
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

bool IPUtilsLinux::addIP4AddressToDevice(const InterfaceConfig& config) {
  const auto parsedAddress =
      parseIPv4InterfaceAddress(config.m_deviceIpv4Address);
  if (!parsedAddress.has_value()) {
    logger.error() << "Invalid IPv4 interface address: "
                   << config.m_deviceIpv4Address;
    return false;
  }

  const QHostAddress& deviceAddress = parsedAddress->address;
  const int prefixLength = parsedAddress->prefixLength;

  const int interfaceIndex = if_nametoindex(WG_INTERFACE);
  if (interfaceIndex == 0) {
    logger.error() << "Failed to get interface index for " << WG_INTERFACE
                   << "error:" << strerror(errno);
    return false;
  }

  // QHostAddress::parseSubnet() cannot be used here: it returns the network
  // address (for example, 10.8.0.0 for 10.8.0.4/24), not the configured host.
  if (!addIPv4Address(interfaceIndex, deviceAddress, prefixLength)) {
    logger.error() << "Failed to set IPv4: " << deviceAddress.toString()
                   << "/" << prefixLength << "error:" << strerror(errno);
    return false;
  }

  return true;
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
