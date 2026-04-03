/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILSLINUX_H
#define DNSUTILSLINUX_H

#include <QDBusInterface>
#include <QScopedPointer>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QHostAddress>
#include <QList>
#include <QString>

#include "daemon/dnsutils.h"
#include "dbustypeslinux.h"

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
  void setLinkDNS(int ifindex, const QList<QHostAddress>& resolvers);
  void setLinkDomains(int ifindex, const QList<DnsLinkDomain>& domains);
  void setLinkDefaultRoute(int ifindex, bool enable);
  void updateLinkDomains();

 private slots:
  void onResolverRegistered();
  void onResolverUnregistered();
  void dnsCallCompleted(QDBusPendingCallWatcher*);
  void dnsDomainsReceived(QDBusPendingCallWatcher*);

 private:
  int m_ifindex = 0;
  int m_domainRetries = 0;
  QMap<int, DnsLinkDomainList> m_linkDomains;
  QScopedPointer<QDBusInterface> m_resolver;
  QString m_pendingIfname;
  QList<QHostAddress> m_pendingResolvers;
};

#endif  // DNSUTILSLINUX_H
