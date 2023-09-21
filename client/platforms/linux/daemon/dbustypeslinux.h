/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DBUSTYPESLINUX_H
#define DBUSTYPESLINUX_H

#include <sys/socket.h>

#include <QByteArray>
#include <QDBusArgument>
#include <QHostAddress>
#include <QtDBus/QtDBus>

/* D-Bus metatype for marshalling arguments to the SetLinkDNS method */
class DnsResolver : public QHostAddress {
 public:
  DnsResolver(const QHostAddress& address = QHostAddress())
      : QHostAddress(address) {}

  friend QDBusArgument& operator<<(QDBusArgument& args, const DnsResolver& ip) {
    args.beginStructure();
    if (ip.protocol() == QAbstractSocket::IPv6Protocol) {
      Q_IPV6ADDR addrv6 = ip.toIPv6Address();
      args << AF_INET6;
      args << QByteArray::fromRawData((const char*)&addrv6, sizeof(addrv6));
    } else {
      quint32 addrv4 = ip.toIPv4Address();
      QByteArray data(4, 0);
      data[0] = (addrv4 >> 24) & 0xff;
      data[1] = (addrv4 >> 16) & 0xff;
      data[2] = (addrv4 >> 8) & 0xff;
      data[3] = (addrv4 >> 0) & 0xff;
      args << AF_INET;
      args << data;
    }
    args.endStructure();
    return args;
  }
  friend const QDBusArgument& operator>>(const QDBusArgument& args,
                                         DnsResolver& ip) {
    int family;
    QByteArray data;
    args.beginStructure();
    args >> family >> data;
    args.endStructure();
    if (family == AF_INET6) {
      ip.setAddress(data.constData());
    } else if (data.count() >= 4) {
      quint32 addrv4 = 0;
      addrv4 |= (data[0] << 24);
      addrv4 |= (data[1] << 16);
      addrv4 |= (data[2] << 8);
      addrv4 |= (data[3] << 0);
      ip.setAddress(addrv4);
    }
    return args;
  }
};
typedef QList<DnsResolver> DnsResolverList;
Q_DECLARE_METATYPE(DnsResolver);
Q_DECLARE_METATYPE(DnsResolverList);

/* D-Bus metatype for marshalling arguments to the SetLinkDomains method */
class DnsLinkDomain {
 public:
  DnsLinkDomain(const QString d = "", bool s = false) {
    domain = d;
    search = s;
  };
  QString domain;
  bool search;

  friend QDBusArgument& operator<<(QDBusArgument& args,
                                   const DnsLinkDomain& data) {
    args.beginStructure();
    args << data.domain << data.search;
    args.endStructure();
    return args;
  }
  friend const QDBusArgument& operator>>(const QDBusArgument& args,
                                         DnsLinkDomain& data) {
    args.beginStructure();
    args >> data.domain >> data.search;
    args.endStructure();
    return args;
  }
  bool operator==(const DnsLinkDomain& other) const {
    return (domain == other.domain) && (search == other.search);
  }
  bool operator==(const QString& other) const { return (domain == other); }
};
typedef QList<DnsLinkDomain> DnsLinkDomainList;
Q_DECLARE_METATYPE(DnsLinkDomain);
Q_DECLARE_METATYPE(DnsLinkDomainList);

/* D-Bus metatype for marshalling the Domains property */
class DnsDomain {
 public:
  DnsDomain() {}
  int ifindex = 0;
  QString domain = "";
  bool search = false;

  friend QDBusArgument& operator<<(QDBusArgument& args, const DnsDomain& data) {
    args.beginStructure();
    args << data.ifindex << data.domain << data.search;
    args.endStructure();
    return args;
  }
  friend const QDBusArgument& operator>>(const QDBusArgument& args,
                                         DnsDomain& data) {
    args.beginStructure();
    args >> data.ifindex >> data.domain >> data.search;
    args.endStructure();
    return args;
  }
};
typedef QList<DnsDomain> DnsDomainList;
Q_DECLARE_METATYPE(DnsDomain);
Q_DECLARE_METATYPE(DnsDomainList);

/* D-Bus metatype for marshalling the freedesktop login manager data. */
class UserData {
 public:
  QString name;
  uint userid;
  QDBusObjectPath path;

  friend QDBusArgument& operator<<(QDBusArgument& args, const UserData& data) {
    args.beginStructure();
    args << data.userid << data.name << data.path;
    args.endStructure();
    return args;
  }
  friend const QDBusArgument& operator>>(const QDBusArgument& args,
                                         UserData& data) {
    args.beginStructure();
    args >> data.userid >> data.name >> data.path;
    args.endStructure();
    return args;
  }
};
typedef QList<UserData> UserDataList;
Q_DECLARE_METATYPE(UserData);
Q_DECLARE_METATYPE(UserDataList);

class DnsMetatypeRegistrationProxy {
 public:
  DnsMetatypeRegistrationProxy() {
    qRegisterMetaType<DnsResolver>();
    qDBusRegisterMetaType<DnsResolver>();
    qRegisterMetaType<DnsResolverList>();
    qDBusRegisterMetaType<DnsResolverList>();
    qRegisterMetaType<DnsLinkDomain>();
    qDBusRegisterMetaType<DnsLinkDomain>();
    qRegisterMetaType<DnsLinkDomainList>();
    qDBusRegisterMetaType<DnsLinkDomainList>();
    qRegisterMetaType<DnsDomain>();
    qDBusRegisterMetaType<DnsDomain>();
    qRegisterMetaType<DnsDomainList>();
    qDBusRegisterMetaType<DnsDomainList>();
    qRegisterMetaType<UserData>();
    qDBusRegisterMetaType<UserData>();
    qRegisterMetaType<UserDataList>();
    qDBusRegisterMetaType<UserDataList>();
  }
};

#endif  // DBUSTYPESLINUX_H
