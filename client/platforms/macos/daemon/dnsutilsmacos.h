/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILSMACOS_H
#define DNSUTILSMACOS_H

#include <systemconfiguration/scdynamicstore.h>
#include <systemconfiguration/systemconfiguration.h>

#include <QHostAddress>
#include <QMap>
#include <QString>

#include "daemon/dnsutils.h"

class DnsUtilsMacos final : public DnsUtils {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DnsUtilsMacos)

 public:
  explicit DnsUtilsMacos(QObject* parent);
  virtual ~DnsUtilsMacos();
  bool updateResolvers(const QString& ifname,
                       const QList<QHostAddress>& resolvers) override;
  bool restoreResolvers() override;

 private:
  void backupResolvers();
  void backupService(const QString& uuid);

 private:
  class DnsBackup {
   public:
    DnsBackup() {}
    bool isValid() const {
      return !m_domain.isEmpty() || !m_search.isEmpty() ||
             !m_servers.isEmpty() || !m_sortlist.isEmpty();
    }

    QString m_domain;
    QStringList m_search;
    QStringList m_servers;
    QStringList m_sortlist;
  };

  SCDynamicStoreRef m_scStore = nullptr;
  QMap<QString, DnsBackup> m_prevServices;
};

#endif  // DNSUTILSMACOS_H
