/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSROUTEMONITOR_H
#define MACOSROUTEMONITOR_H

#include <QByteArray>
#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QSocketNotifier>

#include "ipaddress.h"

struct if_msghdr;
struct rt_msghdr;
struct sockaddr;

class MacosRouteMonitor final : public QObject {
  Q_OBJECT

 public:
  MacosRouteMonitor(const QString& ifname, QObject* parent = nullptr);
  ~MacosRouteMonitor();

  bool insertRoute(const IPAddress& prefix);
  bool deleteRoute(const IPAddress& prefix);
  int interfaceFlags() { return m_ifflags; }

  bool addExclusionRoute(const IPAddress& prefix);
  bool deleteExclusionRoute(const IPAddress& prefix);
  void flushExclusionRoutes();

 private:
  void handleRtmDelete(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleRtmUpdate(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleIfaceInfo(const struct if_msghdr* msg, const QByteArray& payload);
  bool rtmSendRoute(int action, const IPAddress& prefix, unsigned int ifindex,
                    const void* gateway);
  bool rtmFetchRoutes(int family);
  static void rtmAppendAddr(struct rt_msghdr* rtm, size_t maxlen, int rtaddr,
                            const void* sa);
  static QList<QByteArray> parseAddrList(const QByteArray& data);

 private slots:
  void rtsockReady();

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

#endif  // MACOSROUTEMONITOR_H
