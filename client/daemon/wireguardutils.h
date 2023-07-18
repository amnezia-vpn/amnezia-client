/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WIREGUARDUTILS_H
#define WIREGUARDUTILS_H

#include <QCoreApplication>
#include <QHostAddress>
#include <QObject>
#include <QStringList>

#include "interfaceconfig.h"

constexpr const char* WG_INTERFACE = "moz0";

constexpr uint16_t WG_KEEPALIVE_PERIOD = 60;

class WireguardUtils : public QObject {
  Q_OBJECT

 public:
  class PeerStatus {
   public:
    PeerStatus(const QString& pubkey = QString()) { m_pubkey = pubkey; }
    QString m_pubkey;
    qint64 m_handshake = 0;
    qint64 m_rxBytes = 0;
    qint64 m_txBytes = 0;
  };

  explicit WireguardUtils(QObject* parent) : QObject(parent){};
  virtual ~WireguardUtils() = default;

  virtual bool interfaceExists() = 0;
  virtual QString interfaceName() { return WG_INTERFACE; }
  virtual bool addInterface(const InterfaceConfig& config) = 0;
  virtual bool deleteInterface() = 0;

  virtual bool updatePeer(const InterfaceConfig& config) = 0;
  virtual bool deletePeer(const InterfaceConfig& config) = 0;
  virtual QList<PeerStatus> getPeerStatus() = 0;

  virtual bool updateRoutePrefix(const IPAddress& prefix, int hopindex) = 0;
  virtual bool deleteRoutePrefix(const IPAddress& prefix, int hopindex) = 0;

  virtual bool addExclusionRoute(const QHostAddress& address) = 0;
  virtual bool deleteExclusionRoute(const QHostAddress& address) = 0;
};

#endif  // WIREGUARDUTILS_H
