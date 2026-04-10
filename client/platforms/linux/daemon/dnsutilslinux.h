/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILSLINUX_H
#define DNSUTILSLINUX_H

#include <QHostAddress>
#include <QList>
#include <QString>

#include "daemon/dnsutils.h"

class DnsUtilsLinux final : public DnsUtils {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DnsUtilsLinux)

 public:
  DnsUtilsLinux(QObject* parent);
  ~DnsUtilsLinux();
  bool updateResolvers(const QString& ifname,
                       const QList<QHostAddress>& resolvers) override;
  bool restoreResolvers() override;

 private:
  void writeResolvConf(const QList<QHostAddress>& resolvers);
  void restoreResolvConf();

 private:
  QString m_resolvConfOriginal;
  QString m_stateFilePath;
};

#endif  // DNSUTILSLINUX_H
