/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXROUTEMONITOR_H
#define LINUXROUTEMONITOR_H

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QSocketNotifier>

#include "ipaddress.h"

struct if_msghdr;
struct rt_msghdr;
struct sockaddr;

class LinuxRouteMonitor final : public QObject {
  Q_OBJECT

 public:
  LinuxRouteMonitor(const QString& ifname, QObject* parent = nullptr);
  ~LinuxRouteMonitor();

  bool insertRoute(const IPAddress& prefix);
  bool deleteRoute(const IPAddress& prefix);
  int interfaceFlags() { return m_ifflags; }

  bool addExclusionRoute(const IPAddress& prefix);
  bool deleteExclusionRoute(const IPAddress& prefix);
  void flushExclusionRoutes();

 private:
  static QString addrToString(const struct sockaddr* sa);
  static QString addrToString(const QByteArray& data);

  QList<IPAddress> m_exclusionRoutes;
  QByteArray m_defaultGatewayIpv4;
  QByteArray m_defaultGatewayIpv6;
  unsigned int m_defaultIfindexIpv4 = 0;
  unsigned int m_defaultIfindexIpv6 = 0;

  QString m_ifname;
  unsigned int m_ifindex = 0;
  int m_ifflags = 0;
  int m_rtsock = -1;
  int m_rtseq = 0;
  QSocketNotifier* m_notifier = nullptr;
};

#endif  // LINUXROUTEMONITOR_H
