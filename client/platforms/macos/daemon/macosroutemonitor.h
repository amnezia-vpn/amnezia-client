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

  bool insertRoute(const IPAddress& prefix, int flags = 0);
  bool deleteRoute(const IPAddress& prefix, int flags = 0);
  int interfaceFlags() { return m_ifflags; }

  bool addExclusionRoute(const IPAddress& prefix);
  bool deleteExclusionRoute(const IPAddress& prefix);
  void flushExclusionRoutes();

  /*! Keeps an interface-scoped default route pointing at the physical
   *  interface while the tunnel owns the routing table.
   *
   *  The tunnel claims traffic with the 0.0.0.0/1 + 128.0.0.0/1 pair, which
   *  beats the physical default on prefix length. The physical default stays
   *  in the table but is not scoped, so "route -n get -ifscope en0" answers
   *  "not in table" and anything bound to the physical interface - curl
   *  --interface, and every flow the split-tunnel extension pushes back out -
   *  gets "No network route". macOS builds this scoped route itself for VPNs
   *  that go through NEVPNManager; a tunnel that routes on its own has to do
   *  it. */
  void syncPhysicalScopedDefaults();
  void flushPhysicalScopedDefaults();

 private:
  void handleRtmDelete(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleRtmUpdate(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleIfaceInfo(const struct if_msghdr* msg, const QByteArray& payload);
  bool rtmSendRoute(int action, const IPAddress& prefix, unsigned int ifindex,
                    const void* gateway, int flags = 0);
  bool rtmFetchRoutes(int family);
  static void rtmAppendAddr(struct rt_msghdr* rtm, size_t maxlen, int rtaddr,
                            const void* sa);
  static QList<QByteArray> parseAddrList(const QByteArray& data);

 private slots:
  void rtsockReady();

 private:
  static QString addrToString(const struct sockaddr* sa);
  static QString addrToString(const QByteArray& data);

  bool syncPhysicalScopedDefault(int family, const QByteArray& gateway,
                                 unsigned int ifindex);

  QList<IPAddress> m_exclusionRoutes;
  QByteArray m_defaultGatewayIpv4;
  QByteArray m_defaultGatewayIpv6;
  unsigned int m_defaultIfindexIpv4 = 0;
  unsigned int m_defaultIfindexIpv6 = 0;
  /*! Interface the scoped default currently points at, 0 when there is none. */
  unsigned int m_scopedIfindexIpv4 = 0;
  unsigned int m_scopedIfindexIpv6 = 0;

  QString m_ifname;
  unsigned int m_ifindex = 0;
  int m_ifflags = 0;
  int m_rtsock = -1;
  int m_rtseq = 0;
  QSocketNotifier* m_notifier = nullptr;
};

#endif  // MACOSROUTEMONITOR_H
