/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxroutemonitor.h"

#include "router_linux.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <QNetworkInterface>

#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/socket.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QProcess>
#include <QScopeGuard>
#include <QTimer>

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("LinuxRouteMonitor");
}  // namespace

LinuxRouteMonitor::LinuxRouteMonitor(const QString& ifname, QObject* parent)
    : QObject(parent), m_ifname(ifname) {
  MZ_COUNT_CTOR(LinuxRouteMonitor);
  logger.debug() << "LinuxRouteMonitor created.";

  m_rtsock = socket(PF_ROUTE, SOCK_RAW, 0);
  if (m_rtsock < 0) {
    logger.error() << "Failed to create routing socket:" << strerror(errno);
    return;
  }

  RouterLinux &router = RouterLinux::Instance();
  m_defaultGatewayIpv4 = router.getgatewayandiface().toUtf8();

  m_ifindex = if_nametoindex(qPrintable(ifname));
  m_notifier = new QSocketNotifier(m_rtsock, QSocketNotifier::Read, this);
}

LinuxRouteMonitor::~LinuxRouteMonitor() {
  MZ_COUNT_DTOR(LinuxRouteMonitor);
  flushExclusionRoutes();
  if (m_rtsock >= 0) {
    close(m_rtsock);
  }
  logger.debug() << "LinuxRouteMonitor destroyed.";
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
  int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);

  struct ifreq ifc;
  int res;

  if(temp_sock < 0)
    return -1;
  strcpy(ifc.ifr_name, m_ifname.toUtf8());

  res = ioctl(temp_sock, SIOCGIFADDR, &ifc);
  if(res < 0)
    return -1;

  RouterLinux &router = RouterLinux::Instance();
  logger.debug() << "prefix.toString() " << prefix.toString() << " m_ifname " << inet_ntoa(((struct sockaddr_in*)&ifc.ifr_addr)->sin_addr);
  return router.routeAdd(prefix.toString(),   inet_ntoa(((struct sockaddr_in*)&ifc.ifr_addr)->sin_addr), temp_sock);
}

bool LinuxRouteMonitor::deleteRoute(const IPAddress& prefix) {
  int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);

  struct ifreq ifc;
  int res;
  if(temp_sock < 0)
    temp_sock -1;
  strcpy(ifc.ifr_name, m_ifname.toUtf8());

  res = ioctl(temp_sock, SIOCGIFADDR, &ifc);
  if(res < 0)
    return -1;

  RouterLinux &router = RouterLinux::Instance();
  logger.debug() << "prefix.toString() " << prefix.toString() << " m_ifname " <<   inet_ntoa(((struct sockaddr_in*)&ifc.ifr_addr)->sin_addr);
  return router.routeDelete(prefix.toString(),   inet_ntoa(((struct sockaddr_in*)&ifc.ifr_addr)->sin_addr), temp_sock);
}

bool LinuxRouteMonitor::addExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Adding exclusion route for"
                 << logger.sensitive(prefix.toString());

  int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);
  RouterLinux &router = RouterLinux::Instance();
  logger.debug() << "prefix.toString() " << prefix.toString() << " m_defaultGatewayIpv4 " << m_defaultGatewayIpv4;
  return router.routeAdd(prefix.toString(), m_defaultGatewayIpv4, temp_sock);
  // Otherwise, the default route isn't known yet. Do nothing.
  return true;
}

bool LinuxRouteMonitor::deleteExclusionRoute(const IPAddress& prefix) {
  logger.debug() << "Deleting exclusion route for"
                 << logger.sensitive(prefix.toString());

  int temp_sock = socket(AF_INET, SOCK_DGRAM,  IPPROTO_IP);
  RouterLinux &router = RouterLinux::Instance();
  logger.debug() << "prefix.toString() " << prefix.toString() << " m_defaultGatewayIpv4 " << m_defaultGatewayIpv4;
  return router.routeDelete(prefix.toString(), m_defaultGatewayIpv4, temp_sock);
}

void LinuxRouteMonitor::flushExclusionRoutes() {
    RouterLinux &router = RouterLinux::Instance();
    router.clearSavedRoutes();
}
