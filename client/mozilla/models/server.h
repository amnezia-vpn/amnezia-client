/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SERVER_H
#define SERVER_H

#include <QList>
#include <QPair>
#include <QString>

class QJsonObject;

class Server final {
 public:
  Server();
  Server(const QString& countryCode, const QString& cityName);
  Server(const Server& other);
  Server& operator=(const Server& other);
  ~Server();

  [[nodiscard]] bool fromJson(const QJsonObject& obj);
  bool fromMultihop(const Server& exit, const Server& entry);

  static const Server& weightChooser(const QList<Server>& servers);

  bool initialized() const { return !m_hostname.isEmpty(); }

  const QString& hostname() const { return m_hostname; }

  const QString& ipv4AddrIn() const { return m_ipv4AddrIn; }

  const QString& ipv4Gateway() const { return m_ipv4Gateway; }

  const QString& ipv6AddrIn() const { return m_ipv6AddrIn; }

  const QString& ipv6Gateway() const { return m_ipv6Gateway; }

  const QString& publicKey() const { return m_publicKey; }

  const QString& socksName() const { return m_socksName; }

  uint32_t weight() const { return m_weight; }

  uint32_t choosePort() const;

  uint32_t multihopPort() const { return m_multihopPort; }

  const QString& countryCode() const { return m_countryCode; }

  const QString& cityName() const { return m_cityName; }

  bool forcePort(uint32_t port);

  bool operator==(const Server& other) const {
    return m_publicKey == other.m_publicKey;
  }
  // Allow checking against QString, so we can easily search a QList<Server> for
  // a public key.
  bool operator==(const QString& otherPublicKey) const {
    return m_publicKey == otherPublicKey;
  }

 private:
  QString m_hostname;
  QString m_ipv4AddrIn;
  QString m_ipv4Gateway;
  QString m_ipv6AddrIn;
  QString m_ipv6Gateway;
  QList<QPair<uint32_t, uint32_t>> m_portRanges;
  QString m_publicKey;
  QString m_socksName;
  uint32_t m_weight = 0;
  uint32_t m_multihopPort = 0;
  QString m_countryCode;
  QString m_cityName;
};

#endif  // SERVER_H
