/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILSLINUX_H
#define DNSUTILSLINUX_H

#include <QByteArray>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QString>
#include <QStringList>

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

  bool resolvedAvailable() const;
  bool updateResolversFallback(const QList<QHostAddress>& resolvers);
  bool restoreFallbackResolvers();
  bool writeResolvConf(const QByteArray& content);
  QString findResolvconf() const;
  bool runResolvconf(const QString& program, const QStringList& args,
                     const QByteArray& input);

 private slots:
  void dnsCallCompleted(QDBusPendingCallWatcher*);
  void dnsDomainsReceived(QDBusPendingCallWatcher*);

 private:
  int m_ifindex = 0;
  QMap<int, DnsLinkDomainList> m_linkDomains;
  QDBusInterface* m_resolver = nullptr;
  bool m_fallback = false;
  bool m_fallbackActive = false;
  QString m_fallbackBackend;
};

#endif  // DNSUTILSLINUX_H
