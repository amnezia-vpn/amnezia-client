/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IPADDRESSRANGE_H
#define IPADDRESSRANGE_H

#include <QObject>
#include <QString>

class IPAddress;

class IPAddressRange final {
 public:
  enum IPAddressType {
    IPv4,
    IPv6,
  };

  static QList<IPAddressRange> fromIPAddressList(const QList<IPAddress>& list);

  IPAddressRange(const QString& prefix);
  IPAddressRange(const QString& ipAddress, uint32_t range, IPAddressType type);
  IPAddressRange(const IPAddressRange& other);
  IPAddressRange& operator=(const IPAddressRange& other);
  bool operator==(const IPAddressRange& other) const;
  ~IPAddressRange();

  const QString& ipAddress() const { return m_ipAddress; }
  uint32_t range() const { return m_range; }
  IPAddressType type() const { return m_type; }
  const QString toString() const {
    return QString("%1/%2").arg(m_ipAddress).arg(m_range);
  }

 private:
  QString m_ipAddress;
  uint32_t m_range;
  IPAddressType m_type;
};

#endif  // IPADDRESSRANGE_H
