/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILSWINDOWS_H
#define DNSUTILSWINDOWS_H

#include <windows.h>

#include <QHostAddress>
#include <QString>

#include "daemon/dnsutils.h"

class DnsUtilsWindows final : public DnsUtils {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DnsUtilsWindows)

 public:
  explicit DnsUtilsWindows(QObject* parent);
  virtual ~DnsUtilsWindows();
  bool updateResolvers(const QString& ifname,
                       const QList<QHostAddress>& resolvers) override;
  bool restoreResolvers() override;

 private:
  struct InterfaceMetricState {
    bool valid = false;
    bool automatic = false;
    ULONG metric = 0;
  };

  quint64 m_luid = 0;
  InterfaceMetricState m_ipv4Metric;
  InterfaceMetricState m_ipv6Metric;
  DWORD (*m_setInterfaceDnsSettingsProcAddr)(GUID, const void*) = nullptr;

  void preferInterfaceMetric(int family, InterfaceMetricState& state);
  void restoreInterfaceMetric(int family, InterfaceMetricState& state);
  bool updateResolversWin32(GUID, const QList<QHostAddress>& resolvers);
  bool updateResolversNetsh(int ifindex, const QList<QHostAddress>& resolvers);
};

#endif  // DNSUTILSWINDOWS_H
