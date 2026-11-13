/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WILDCARDROUTETABLE_H
#define WILDCARDROUTETABLE_H

#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QString>

// Addresses learned from DNS answers for wildcard site entries.
//
// A learned route is kept for the whole session, not for the TTL of the
// answer that justified it: an open connection to the address keeps using it
// after the TTL, and the system resolver may keep serving the record from its
// cache past any lifetime the daemon picks. Withdrawing the route at the TTL
// would cut the first and silently send the second through the VPN.
//
// The TTL still matters: once it has passed (plus a grace period, so a cached
// answer and the connections opened from it have run out), the entry is
// "stale" and becomes a candidate for eviction when a pattern, or the table as
// a whole, runs out of room. Stale entries are evicted least recently seen first, so a busy pattern
// recycles its own old addresses instead of being refused, and one pattern
// cannot take the room another pattern needs.
class WildcardRouteTable {
 public:
  enum class AddResult {
    Install,
    Renew,
    Rejected,
    AtCapacity,
  };

  explicit WildcardRouteTable(int maxEntries = 2048,
                              int maxEntriesPerPattern = 512);

  // Addresses that must never be routed around the tunnel: the forwarder's
  // own endpoint and upstream resolvers, the tunnel's addresses and gateways,
  // the server endpoint.
  void setReserved(const QList<QHostAddress>& reserved,
                   const QList<QPair<QHostAddress, int>>& reservedSubnets = {});
  bool isReserved(const QHostAddress& address) const;

  static qint64 monotonicNowMsec();

  // Adds or renews `address` for `pattern`. On Install, `evicted` (when not
  // null) receives the stale address that was dropped to make room, or a null
  // address when nothing was evicted. The caller withdraws the evicted route.
  AddResult add(const QHostAddress& address, const QString& pattern,
                quint32 ttlSeconds, qint64 nowMsec,
                QHostAddress* evicted = nullptr);

  QList<QHostAddress> takeAll();
  QList<QHostAddress> addresses() const { return m_entries.keys(); }

  bool remove(const QHostAddress& address);

  bool contains(const QHostAddress& address) const;
  bool isStale(const QHostAddress& address, qint64 nowMsec) const;
  int count() const { return static_cast<int>(m_entries.count()); }
  int countForPattern(const QString& pattern) const {
    return m_perPattern.value(pattern);
  }

  static bool isRoutable(const QHostAddress& address);

  static constexpr quint32 MinimumTtlSeconds = 30;
  // The forwarded answer keeps its TTL, so the system cache may serve it that
  // long; an entry becomes evictable only after that plus this grace period.
  // There is no upper clamp: a day-long TTL keeps its route a day.
  static constexpr qint64 StaleGraceMsec = 60 * 60 * 1000;

 private:
  struct Entry {
    QString pattern;
    qint64 freshUntilMsec = 0;
    qint64 lastSeenMsec = 0;
  };

  // Finds the least recently seen stale entry, restricted to `pattern` when
  // it is not empty. Returns a null address when there is none.
  QHostAddress findEvictable(const QString& pattern, qint64 nowMsec) const;
  void erase(const QHostAddress& address);

  QHash<QHostAddress, Entry> m_entries;
  QHash<QString, int> m_perPattern;
  QList<QHostAddress> m_reserved;
  QList<QPair<QHostAddress, int>> m_reservedSubnets;
  int m_maxEntries;
  int m_maxEntriesPerPattern;
};

#endif  // WILDCARDROUTETABLE_H
