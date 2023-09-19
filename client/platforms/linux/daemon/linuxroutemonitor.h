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


class LinuxRouteMonitor final : public QObject {
  Q_OBJECT

 public:
  LinuxRouteMonitor(const QString& ifname, QObject* parent = nullptr);
  ~LinuxRouteMonitor();

  bool insertRoute(const IPAddress& prefix);
  bool deleteRoute(const IPAddress& prefix);

  bool addExclusionRoute(const IPAddress& prefix);
  bool deleteExclusionRoute(const IPAddress& prefix);
 private:
  static QString addrToString(const struct sockaddr* sa);
  static QString addrToString(const QByteArray& data);
  bool rtmSendRoute(int action, int flags, int type,
                    const IPAddress& prefix);
  QString getgatewayandiface();
  QString m_ifname;
  unsigned int m_ifindex = 0;
  int m_nlsock = -1;
  int m_nlseq = 0;
  QSocketNotifier* m_notifier = nullptr;

 private slots:
    void nlsockReady();

};

#endif  // LINUXROUTEMONITOR_H
