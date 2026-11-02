/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WILDCARDROUTETABLE_H
#define WILDCARDROUTETABLE_H

#include <QHash>
#include <QHostAddress>
#include <QList>

class WildcardRouteTable {
 public:
  enum class AddResult {
    Install,
    Renew,
    Rejected,
    AtCapacity,
  };

  explicit WildcardRouteTable(int maxEntries = 2048);

  void setReserved(const QList<QHostAddress>& reserved);

  static qint64 monotonicNowMsec();

  AddResult add(const QHostAddress& address, quint32 ttlSeconds,
                qint64 nowMsec);

  QList<QHostAddress> expire(qint64 nowMsec);

  QList<QHostAddress> takeAll();

  bool remove(const QHostAddress& address);

  bool contains(const QHostAddress& address) const;
  int count() const { return static_cast<int>(m_entries.count()); }
  bool isAtCapacity() const { return count() >= m_maxEntries; }

  static bool isRoutable(const QHostAddress& address);

  static constexpr quint32 MinimumTtlSeconds = 30;
  static constexpr quint32 MaximumTtlSeconds = 3600;

 private:
  QHash<QHostAddress, qint64> m_entries;
  QList<QHostAddress> m_reserved;
  int m_maxEntries;
};

#endif  // WILDCARDROUTETABLE_H
