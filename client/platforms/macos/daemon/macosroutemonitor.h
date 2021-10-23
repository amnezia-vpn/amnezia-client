/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSROUTEMONITOR_H
#define MACOSROUTEMONITOR_H

#include "ipaddressrange.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSocketNotifier>

struct if_msghdr;
struct rt_msghdr;
struct sockaddr;

class MacosRouteMonitor final : public QObject {
  Q_OBJECT

 public:
  MacosRouteMonitor(const QString& ifname, QObject* parent = nullptr);
  ~MacosRouteMonitor();

  bool insertRoute(const IPAddressRange& prefix);
  bool deleteRoute(const IPAddressRange& prefix);
  int interfaceFlags() { return m_ifflags; }

 private:
  void handleRtmAdd(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleRtmDelete(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleRtmChange(const struct rt_msghdr* msg, const QByteArray& payload);
  void handleIfaceInfo(const struct if_msghdr* msg, const QByteArray& payload);
  bool rtmSendRoute(int action, const IPAddressRange& prefix);
  static void rtmAppendAddr(struct rt_msghdr* rtm, size_t maxlen, int rtaddr,
                            const void* sa);
  static QList<QByteArray> parseAddrList(const QByteArray& data);

 private slots:
  void rtsockReady();

 private:
  static QString addrToString(const struct sockaddr* sa);
  static QString addrToString(const QByteArray& data);

  QString m_ifname;
  int m_ifflags = 0;
  int m_rtsock = -1;
  int m_rtseq = 0;
  QSocketNotifier* m_notifier = nullptr;
};

#endif  // MACOSROUTEMONITOR_H
