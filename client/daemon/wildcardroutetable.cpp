/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wildcardroutetable.h"

#include <chrono>

qint64 WildcardRouteTable::monotonicNowMsec() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

WildcardRouteTable::WildcardRouteTable(int maxEntries)
    : m_maxEntries(maxEntries > 0 ? maxEntries : 1) {}

void WildcardRouteTable::setReserved(const QList<QHostAddress>& reserved) {
  m_reserved.clear();
  for (const QHostAddress& address : reserved) {
    if (!address.isNull()) {
      m_reserved.append(address);
    }
  }
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
    const QHostAddress& address, quint32 ttlSeconds, qint64 nowMsec) {
  if (!isRoutable(address)) {
    return AddResult::Rejected;
  }
  for (const QHostAddress& reserved : m_reserved) {
    if (reserved.isEqual(address)) {
      return AddResult::Rejected;
    }
  }

  quint32 lifetime = ttlSeconds;
  if (lifetime < MinimumTtlSeconds) {
    lifetime = MinimumTtlSeconds;
  } else if (lifetime > MaximumTtlSeconds) {
    lifetime = MaximumTtlSeconds;
  }
  const qint64 expiry = nowMsec + static_cast<qint64>(lifetime) * 1000;

  auto existing = m_entries.find(address);
  if (existing != m_entries.end()) {
    if (expiry > existing.value()) {
      existing.value() = expiry;
    }
    return AddResult::Renew;
  }

  if (isAtCapacity()) {
    return AddResult::AtCapacity;
  }

  m_entries.insert(address, expiry);
  return AddResult::Install;
}

QList<QHostAddress> WildcardRouteTable::expire(qint64 nowMsec) {
  QList<QHostAddress> gone;
  for (auto i = m_entries.begin(); i != m_entries.end();) {
    if (i.value() <= nowMsec) {
      gone.append(i.key());
      i = m_entries.erase(i);
    } else {
      ++i;
    }
  }
  return gone;
}

QList<QHostAddress> WildcardRouteTable::takeAll() {
  const QList<QHostAddress> all = m_entries.keys();
  m_entries.clear();
  return all;
}

bool WildcardRouteTable::remove(const QHostAddress& address) {
  return m_entries.remove(address) > 0;
}

bool WildcardRouteTable::contains(const QHostAddress& address) const {
  return m_entries.contains(address);
}
