/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ipaddress.h"
#include "bigintipv6addr.h"

#include <QtMath>

namespace {

quint32 s_allIpV4Ones = static_cast<quint32>(qPow(2, 32) - 1);

BigIntIPv6Addr s_allIPv6Ones;
bool s_ipv6Initialized = false;

void maybeInitialize() {
  if (s_ipv6Initialized) return;

  s_ipv6Initialized = true;

  Q_IPV6ADDR allOnes;
  memset((void*)&allOnes, static_cast<quint8>(qPow(2, 8) - 1), sizeof(allOnes));

  s_allIPv6Ones = BigIntIPv6Addr(allOnes);
}

}  // namespace

// static
IPAddress IPAddress::create(const QString& ip) {
  if (ip.contains("/")) {
    QPair<QHostAddress, int> p = QHostAddress::parseSubnet(ip);

    if (p.first.protocol() == QAbstractSocket::IPv4Protocol) {
      if (p.second < 32) {
        return IPAddress(p.first, p.second);
      }
      return IPAddress(p.first);
    }

    if (p.first.protocol() == QAbstractSocket::IPv6Protocol) {
      if (p.second < 128) {
        return IPAddress(p.first, p.second);
      }
      return IPAddress(p.first);
    }

    Q_ASSERT(false);
  }

  return IPAddress(QHostAddress(ip));
}

IPAddress::IPAddress() {
  maybeInitialize();
}

IPAddress::IPAddress(const IPAddress& other) {
  maybeInitialize();
  *this = other;
}

IPAddress& IPAddress::operator=(const IPAddress& other) {
  if (this == &other) return *this;

  m_address = other.m_address;
  m_prefixLength = other.m_prefixLength;
  m_netmask = other.m_netmask;
  m_hostmask = other.m_hostmask;
  m_broadcastAddress = other.m_broadcastAddress;

  return *this;
}

IPAddress::IPAddress(const QHostAddress& address)
    : m_address(address), m_broadcastAddress(address) {
  maybeInitialize();

  if (address.protocol() == QAbstractSocket::IPv4Protocol) {
    m_prefixLength = 32;
    m_netmask = QHostAddress(s_allIpV4Ones);
    m_hostmask = QHostAddress((quint32)(0));
  } else {
    Q_ASSERT(address.protocol() == QAbstractSocket::IPv6Protocol);
    m_prefixLength = 128;

    m_netmask = QHostAddress(s_allIPv6Ones.value());

    {
      Q_IPV6ADDR ipv6;
      memset((void*)&ipv6, 0, sizeof(ipv6));
      m_hostmask = QHostAddress(ipv6);
    }
  }
}

IPAddress::IPAddress(const QHostAddress& address, int prefixLength)
    : m_address(address), m_prefixLength(prefixLength) {
  maybeInitialize();

  if (address.protocol() == QAbstractSocket::IPv4Protocol) {
    Q_ASSERT(prefixLength >= 0 && prefixLength <= 32);
    m_netmask = QHostAddress(s_allIpV4Ones ^ (s_allIpV4Ones >> prefixLength));
    m_hostmask = QHostAddress(m_netmask.toIPv4Address() ^ s_allIpV4Ones);
    m_broadcastAddress =
        QHostAddress(address.toIPv4Address() | m_hostmask.toIPv4Address());
  } else {
    Q_ASSERT(address.protocol() == QAbstractSocket::IPv6Protocol);
    Q_ASSERT(prefixLength >= 0 && prefixLength <= 128);

    Q_IPV6ADDR netmask;
    {
      BigIntIPv6Addr tmp = (s_allIPv6Ones >> prefixLength);
      for (int i = 0; i < 16; ++i)
        netmask[i] = s_allIPv6Ones.value()[i] ^ tmp.value()[i];
    }
    m_netmask = QHostAddress(netmask);

    {
      Q_IPV6ADDR tmp;
      for (int i = 0; i < 16; ++i)
        tmp[i] = netmask[i] ^ s_allIPv6Ones.value()[i];
      m_hostmask = QHostAddress(tmp);
    }

    {
      Q_IPV6ADDR ipv6Address = address.toIPv6Address();
      Q_IPV6ADDR ipv6Hostname = m_hostmask.toIPv6Address();
      for (int i = 0; i < 16; ++i) ipv6Address[i] |= ipv6Hostname[i];
      m_broadcastAddress = QHostAddress(ipv6Address);
    }
  }
}

IPAddress::~IPAddress() { }

QAbstractSocket::NetworkLayerProtocol IPAddress::type() const {
  return m_address.protocol();
}

bool IPAddress::overlaps(const IPAddress& other) const {
  return other.contains(m_address) || other.contains(m_broadcastAddress) ||
         contains(other.m_address) || contains(other.m_broadcastAddress);
}

bool IPAddress::contains(const QHostAddress& address) const {
  if (address.protocol() != m_address.protocol()) {
    return false;
  }

  if (address.protocol() == QAbstractSocket::IPv4Protocol) {
    return (m_address.toIPv4Address() <= address.toIPv4Address()) &&
           (address.toIPv4Address() <= m_broadcastAddress.toIPv4Address());
  }

  Q_ASSERT(address.protocol() == QAbstractSocket::IPv6Protocol);
  return (BigIntIPv6Addr(m_address.toIPv6Address()) <=
          BigIntIPv6Addr(address.toIPv6Address())) &&
         (BigIntIPv6Addr(address.toIPv6Address()) <=
          BigIntIPv6Addr(m_broadcastAddress.toIPv6Address()));
}

bool IPAddress::operator==(const IPAddress& other) const {
  return m_address == other.m_address && m_netmask == other.m_netmask;
}

bool IPAddress::subnetOf(const IPAddress& other) const {
  if (other.m_address.protocol() != m_address.protocol()) {
    return false;
  }

  if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    return other.m_address.toIPv4Address() <= m_address.toIPv4Address() &&
           other.m_broadcastAddress.toIPv4Address() >=
               m_broadcastAddress.toIPv4Address();
  }

  Q_ASSERT(m_address.protocol() == QAbstractSocket::IPv6Protocol);
  return BigIntIPv6Addr(other.m_address.toIPv6Address()) <=
             BigIntIPv6Addr(m_address.toIPv6Address()) &&
         BigIntIPv6Addr(other.m_broadcastAddress.toIPv6Address()) >=
             BigIntIPv6Addr(m_broadcastAddress.toIPv6Address());
}

QList<IPAddress> IPAddress::subnets() const {
  QList<IPAddress> list;

  if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    if (m_prefixLength == 32) {
      list.append(*this);
      return list;
    }

    quint64 start = m_address.toIPv4Address();
    quint64 end = quint64(m_broadcastAddress.toIPv4Address()) + 1;
    quint64 step = ((quint64)m_hostmask.toIPv4Address() + 1) >> 1;

    while (start < end) {
      int newPrefixLength = m_prefixLength + 1;
      if (newPrefixLength == 32) {
        list.append(IPAddress(QHostAddress(static_cast<quint32>(start))));
      } else {
        list.append(IPAddress(QHostAddress(static_cast<quint32>(start)),
                              m_prefixLength + 1));
      }
      start += step;
    }

    return list;
  }

  Q_ASSERT(m_address.protocol() == QAbstractSocket::IPv6Protocol);

  if (m_prefixLength == 128) {
    list.append(*this);
    return list;
  }

  BigInt start(17);
  {
    Q_IPV6ADDR addr = m_address.toIPv6Address();
    for (int i = 0; i < 16; ++i) {
      start.setValueAt(addr[i], i + 1);
    }
  }

  BigInt end(17);
  {
    Q_IPV6ADDR addr = m_broadcastAddress.toIPv6Address();
    for (int i = 0; i < 16; ++i) {
      end.setValueAt(addr[i], i + 1);
    }
    ++end;
  }

  BigInt step(17);
  {
    Q_IPV6ADDR addr = m_hostmask.toIPv6Address();
    for (int i = 0; i < 16; ++i) {
      step.setValueAt(addr[i], i + 1);
    }
    step = (++step) >> 1;
  }

  while (start < end) {
    int newPrefixLength = m_prefixLength + 1;
    Q_IPV6ADDR startIPv6;
    for (int i = 0; i < 16; ++i) {
      startIPv6[i] = start.valueAt(i + 1);
    }

    if (newPrefixLength == 128) {
      list.append(IPAddress(QHostAddress(startIPv6)));
    } else {
      list.append(IPAddress(QHostAddress(startIPv6), m_prefixLength + 1));
    }
    start += step;
  }

  return list;
}

// static
QList<IPAddress> IPAddress::excludeAddresses(
    const QList<IPAddress>& sourceList, const QList<IPAddress>& excludeList) {
  QList<IPAddress> results = sourceList;

  for (const IPAddress& exclude : excludeList) {
    QList<IPAddress> newResults;

    for (const IPAddress& ip : results) {
      if (ip.overlaps(exclude)) {
        QList<IPAddress> range = ip.excludeAddresses(exclude);
        newResults.append(range);
      } else {
        newResults.append(ip);
      }
    }

    results = newResults;
  }

  return results;
}

QList<IPAddress> IPAddress::excludeAddresses(const IPAddress& ip) const {
  QList<IPAddress> sn = subnets();
  Q_ASSERT(sn.length() >= 2);

  QList<IPAddress> result;
  while (sn[0] != ip && sn[1] != ip) {
    if (ip.subnetOf(sn[0])) {
      result.append(sn[1]);
      sn = sn[0].subnets();
    } else if (ip.subnetOf(sn[1])) {
      result.append(sn[0]);
      sn = sn[1].subnets();
    } else {
      Q_ASSERT(false);
    }
  }

  if (sn[0] == ip) {
    result.append(sn[1]);
  } else if (sn[1] == ip) {
    result.append(sn[0]);
  } else {
    Q_ASSERT(false);
  }

  return result;
}
