/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ipaddress.h"

#include <QtMath>

#include "leakdetector.h"

IPAddress::IPAddress() { MZ_COUNT_CTOR(IPAddress); }

IPAddress::IPAddress(const QString& ip) {
  MZ_COUNT_CTOR(IPAddress);
  if (ip.contains("/")) {
    QPair<QHostAddress, int> p = QHostAddress::parseSubnet(ip);
    m_address = p.first;
    m_prefixLength = p.second;
  } else {
    m_address = QHostAddress(ip);
    m_prefixLength = 999999;
  }

  if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    if (m_prefixLength >= 32) {
      m_prefixLength = 32;
    }
  } else if (m_address.protocol() == QAbstractSocket::IPv6Protocol) {
    if (m_prefixLength >= 128) {
      m_prefixLength = 128;
    }
  } else {
    Q_ASSERT(false);
  }
}

IPAddress::IPAddress(const IPAddress& other) {
  MZ_COUNT_CTOR(IPAddress);
  *this = other;
}

IPAddress& IPAddress::operator=(const IPAddress& other) {
  if (this == &other) return *this;

  m_address = other.m_address;
  m_prefixLength = other.m_prefixLength;

  return *this;
}

IPAddress::IPAddress(const QHostAddress& address) : m_address(address) {
  MZ_COUNT_CTOR(IPAddress);

  if (address.protocol() == QAbstractSocket::IPv4Protocol) {
    m_prefixLength = 32;
  } else {
    Q_ASSERT(address.protocol() == QAbstractSocket::IPv6Protocol);
    m_prefixLength = 128;
  }
}

IPAddress::IPAddress(const QHostAddress& address, int prefixLength)
    : m_address(address), m_prefixLength(prefixLength) {
  MZ_COUNT_CTOR(IPAddress);

  if (address.protocol() == QAbstractSocket::IPv4Protocol) {
    Q_ASSERT(prefixLength >= 0 && prefixLength <= 32);
  } else {
    Q_ASSERT(address.protocol() == QAbstractSocket::IPv6Protocol);
    Q_ASSERT(prefixLength >= 0 && prefixLength <= 128);
  }
}

IPAddress::~IPAddress() { MZ_COUNT_DTOR(IPAddress); }

QAbstractSocket::NetworkLayerProtocol IPAddress::type() const {
  return m_address.protocol();
}

QHostAddress IPAddress::netmask() const {
  if (m_address.protocol() == QAbstractSocket::IPv6Protocol) {
    Q_IPV6ADDR rawNetmask = {0};
    Q_ASSERT(m_prefixLength <= 128);
    memset(&rawNetmask, 0xff, m_prefixLength / 8);
    if (m_prefixLength % 8) {
      rawNetmask[m_prefixLength / 8] = 0xFF ^ (0xFF >> (m_prefixLength % 8));
    }
    return QHostAddress(rawNetmask);
  } else if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    quint32 rawNetmask = 0xffffffff;
    Q_ASSERT(m_prefixLength <= 32);
    if (m_prefixLength < 32) {
      rawNetmask ^= (0xffffffff >> m_prefixLength);
    }
    return QHostAddress(rawNetmask);
  } else {
    return QHostAddress();
  }
}

QHostAddress IPAddress::hostmask() const {
  if (m_address.protocol() == QAbstractSocket::IPv6Protocol) {
    Q_IPV6ADDR rawHostmask = {0};
    int offset = (m_prefixLength + 7) / 8;
    Q_ASSERT(m_prefixLength <= 128);
    memset(&rawHostmask[offset], 0xff, sizeof(rawHostmask) - offset);
    if (m_prefixLength % 8) {
      rawHostmask[m_prefixLength / 8] = 0xFF >> (m_prefixLength % 8);
    }
    return QHostAddress(rawHostmask);
  } else if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    if (m_prefixLength < 32) {
      return QHostAddress(0xffffffff >> m_prefixLength);
    } else {
      quint32 zero = 0;
      return QHostAddress(zero);
    }
  } else {
    return QHostAddress();
  }
}

QHostAddress IPAddress::broadcastAddress() const {
  if (m_address.protocol() == QAbstractSocket::IPv6Protocol) {
    Q_IPV6ADDR rawAddress = m_address.toIPv6Address();
    int offset = (m_prefixLength + 7) / 8;
    memset(&rawAddress[offset], 0xff, sizeof(rawAddress) - offset);
    if (m_prefixLength % 8) {
      rawAddress[m_prefixLength / 8] |= 0xFF >> (m_prefixLength % 8);
    }
    return QHostAddress(rawAddress);
  } else if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    quint32 rawAddress = m_address.toIPv4Address();
    if (m_prefixLength < 32) {
      rawAddress |= (0xffffffff >> m_prefixLength);
    }
    return QHostAddress(rawAddress);
  } else {
    return QHostAddress();
  }
}

bool IPAddress::overlaps(const IPAddress& other) const {
  if (m_prefixLength < other.m_prefixLength) {
    return contains(other.m_address);
  } else {
    return other.contains(m_address);
  }
}

bool IPAddress::contains(const QHostAddress& address) const {
  if (address.protocol() != m_address.protocol()) {
    return false;
  }
  if (m_prefixLength == 0) {
    return true;
  }

  if (m_address.protocol() == QAbstractSocket::IPv6Protocol) {
    Q_IPV6ADDR a = m_address.toIPv6Address();
    Q_IPV6ADDR b = address.toIPv6Address();
    int bytes = m_prefixLength / 8;
    if (bytes > 0) {
      if (memcmp(&a, &b, bytes) != 0) {
        return false;
      }
    }

    if (m_prefixLength % 8) {
      quint8 diff = (a[bytes] ^ b[bytes]) >> (8 - m_prefixLength % 8);
      return (diff == 0);
    }

    return true;
  }

  if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    quint32 diff = m_address.toIPv4Address() ^ address.toIPv4Address();
    if (m_prefixLength < 32) {
      diff >>= (32 - m_prefixLength);
    }
    return (diff == 0);
  }

  return false;
}

bool IPAddress::operator==(const IPAddress& other) const {
  return m_address == other.m_address && m_prefixLength == other.m_prefixLength;
}

bool IPAddress::subnetOf(const IPAddress& other) const {
  if (other.m_address.protocol() != m_address.protocol()) {
    return false;
  }
  if (m_prefixLength < other.m_prefixLength) {
    return false;
  }

  return other.contains(m_address);
}

QList<IPAddress> IPAddress::subnets() const {
  QList<IPAddress> list;

  if (m_address.protocol() == QAbstractSocket::IPv4Protocol) {
    if (m_prefixLength >= 32) {
      list.append(*this);
      return list;
    }

    quint32 rawAddress = m_address.toIPv4Address();
    list.append(IPAddress(QHostAddress(rawAddress), m_prefixLength + 1));

    rawAddress |= (0x80000000 >> m_prefixLength);
    list.append(IPAddress(QHostAddress(rawAddress), m_prefixLength + 1));

    return list;
  }

  Q_ASSERT(m_address.protocol() == QAbstractSocket::IPv6Protocol);

  if (m_prefixLength >= 128) {
    list.append(*this);
    return list;
  }

  Q_IPV6ADDR rawAddress = m_address.toIPv6Address();
  list.append(IPAddress(QHostAddress(rawAddress), m_prefixLength + 1));

  rawAddress[m_prefixLength / 8] |= (0x80 >> (m_prefixLength % 8));
  list.append(IPAddress(QHostAddress(rawAddress), m_prefixLength + 1));

  return list;
}

// static
QList<IPAddress> IPAddress::excludeAddresses(
    const QList<IPAddress>& sourceList, const QList<IPAddress>& excludeList) {
  QList<IPAddress> results = sourceList;

  for (const IPAddress& exclude : excludeList) {
    QList<IPAddress> newResults;

    for (const IPAddress& ip : results) {
      if (!ip.overlaps(exclude)) {
        newResults.append(ip);
      } else if (exclude.subnetOf(ip) && exclude != ip) {
        QList<IPAddress> range = ip.excludeAddresses(exclude);
        newResults.append(range);
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
