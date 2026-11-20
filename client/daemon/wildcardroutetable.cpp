/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wildcardroutetable.h"

#include <chrono>

qint64 WildcardRouteTable::monotonicNowMsec() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

WildcardRouteTable::WildcardRouteTable(int maxEntries, int maxEntriesPerPattern)
    : m_maxEntries(maxEntries > 0 ? maxEntries : 1),
      m_maxEntriesPerPattern(maxEntriesPerPattern > 0 ? maxEntriesPerPattern
                                                      : 1) {}

void WildcardRouteTable::setReserved(
    const QList<QHostAddress>& reserved,
    const QList<QPair<QHostAddress, int>>& reservedSubnets) {
  m_reserved.clear();
  m_reservedSubnets.clear();
  for (const QPair<QHostAddress, int>& subnet : reservedSubnets) {
    if (!subnet.first.isNull() && subnet.second >= 0) {
      m_reservedSubnets.append(subnet);
    }
  }
  for (const QHostAddress& address : reserved) {
    if (!address.isNull() && !isReserved(address)) {
      m_reserved.append(address);
    }
  }
}

bool WildcardRouteTable::isReserved(const QHostAddress& address) const {
  for (const QHostAddress& reserved : m_reserved) {
    if (reserved.isEqual(address)) {
      return true;
    }
  }
  for (const QPair<QHostAddress, int>& subnet : m_reservedSubnets) {
    if (address.isInSubnet(subnet)) {
      return true;
    }
  }
  return false;
}

bool WildcardRouteTable::isRoutable(const QHostAddress& address) {
  if (address.isNull() || address.isLoopback() || address.isMulticast() ||
      address.isBroadcast() || address.isLinkLocal()) {
    return false;
  }
  if (address == QHostAddress(QHostAddress::AnyIPv4) ||
      address == QHostAddress(QHostAddress::AnyIPv6)) {
    return false;
  }
  return true;
}

WildcardRouteTable::AddResult WildcardRouteTable::add(
    const QHostAddress& address, const QString& pattern, quint32 ttlSeconds,
    qint64 nowMsec, QHostAddress* evicted) {
  if (evicted) {
    *evicted = QHostAddress();
  }
  if (!isRoutable(address) || isReserved(address)) {
    return AddResult::Rejected;
  }

  const quint32 lifetime = qMax(ttlSeconds, MinimumTtlSeconds);
  const qint64 freshUntil =
      nowMsec + static_cast<qint64>(lifetime) * 1000 + StaleGraceMsec;

  auto existing = m_entries.find(address);
  if (existing != m_entries.end()) {
    if (freshUntil > existing->freshUntilMsec) {
      existing->freshUntilMsec = freshUntil;
    }
    existing->lastSeenMsec = nowMsec;
    return AddResult::Renew;
  }

  // Make room: first within the pattern's own budget, then in the table.
  QHostAddress victim;
  if (m_perPattern.value(pattern) >= m_maxEntriesPerPattern) {
    victim = findEvictable(pattern, nowMsec);
    if (victim.isNull()) {
      return AddResult::AtCapacity;
    }
  } else if (count() >= m_maxEntries) {
    victim = findEvictable(QString(), nowMsec);
    if (victim.isNull()) {
      return AddResult::AtCapacity;
    }
  }
  if (!victim.isNull()) {
    erase(victim);
    if (evicted) {
      *evicted = victim;
    }
  }

  m_entries.insert(address, Entry{pattern, freshUntil, nowMsec});
  m_perPattern[pattern] += 1;
  return AddResult::Install;
}

QHostAddress WildcardRouteTable::findEvictable(const QString& pattern,
                                               qint64 nowMsec) const {
  QHostAddress best;
  qint64 bestSeen = 0;
  for (auto i = m_entries.constBegin(); i != m_entries.constEnd(); ++i) {
    if (!pattern.isEmpty() && i->pattern != pattern) {
      continue;
    }
    if (i->freshUntilMsec > nowMsec) {
      continue;
    }
    if (best.isNull() || i->lastSeenMsec < bestSeen) {
      best = i.key();
      bestSeen = i->lastSeenMsec;
    }
  }
  return best;
}

void WildcardRouteTable::erase(const QHostAddress& address) {
  auto i = m_entries.find(address);
  if (i == m_entries.end()) {
    return;
  }
  auto counter = m_perPattern.find(i->pattern);
  if (counter != m_perPattern.end() && --counter.value() <= 0) {
    m_perPattern.erase(counter);
  }
  m_entries.erase(i);
}

QList<QHostAddress> WildcardRouteTable::takeAll() {
  const QList<QHostAddress> all = m_entries.keys();
  m_entries.clear();
  m_perPattern.clear();
  return all;
}

bool WildcardRouteTable::remove(const QHostAddress& address) {
  if (!m_entries.contains(address)) {
    return false;
  }
  erase(address);
  return true;
}

bool WildcardRouteTable::contains(const QHostAddress& address) const {
  return m_entries.contains(address);
}

bool WildcardRouteTable::isStale(const QHostAddress& address,
                                 qint64 nowMsec) const {
  auto i = m_entries.constFind(address);
  return i != m_entries.constEnd() && i->freshUntilMsec <= nowMsec;
}
