/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSUTILSMACOS_H
#define DNSUTILSMACOS_H

#include <systemconfiguration/scdynamicstore.h>
#include <systemconfiguration/systemconfiguration.h>

#include <dispatch/dispatch.h>

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
  struct NotificationContext;
  static void dnsChanged(SCDynamicStoreRef, CFArrayRef, void* info);
  bool startMonitoring();
  void stopMonitoring();
  bool applyResolvers();
  bool backupService(const QString& uuid);

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
  NotificationContext* m_notificationContext = nullptr;
  dispatch_queue_t m_notificationQueue = nullptr;
  CFDictionaryRef m_dnsConfig = nullptr;
  quint64 m_generation = 0;
  bool m_monitoring = false;
};

#endif  // DNSUTILSMACOS_H
