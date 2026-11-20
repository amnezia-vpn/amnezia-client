/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DAEMON_H
#define DAEMON_H

#include <QDateTime>
#include <QTimer>

#include "daemon/daemonerrors.h"
#include "daemonerrors.h"
#include "dnsutils.h"
#include "dnswildcardrouter.h"
#include "wildcardroutetable.h"
#include "interfaceconfig.h"
#include "iputils.h"
#include "wireguardutils.h"

class Daemon : public QObject {
  Q_OBJECT

 public:
  enum Op {
    Up,
    Down,
    Switch,
  };

  explicit Daemon(QObject* parent);
  ~Daemon();

  static Daemon* instance();

  static bool parseConfig(const QJsonObject& obj, InterfaceConfig& config);

  virtual bool activate(const InterfaceConfig& config);
  virtual bool deactivate(bool emitSignals = true);
  virtual QJsonObject getStatus();

  // Callback before any Activating measure is done
  virtual void prepareActivation(const InterfaceConfig& config, int inetAdapterIndex = 0) {
      Q_UNUSED(config)  };
  virtual void activateSplitTunnel(const InterfaceConfig& config, int vpnAdapterIndex = 0) {
      Q_UNUSED(config)  };

  QString logs();
  void cleanLogs();

 signals:
  void connected(const QString& pubkey);
  void activationFailure();
  void disconnected();
  void backendFailure(DaemonError reason = DaemonError::ERROR_FATAL);

 private:
  bool maybeUpdateResolvers(const InterfaceConfig& config);
  bool addExclusionRoute(const IPAddress& address);
  bool delExclusionRoute(const IPAddress& address);
  void reconcileDnsWildcardRouter(const InterfaceConfig& config);
  void onWildcardAddressesResolved(const QString& name,
                                   const QList<DnsWildcardRouter::Record>& records);
  void withdrawWildcardRoutes();
  void withdrawWildcardAddress(const QHostAddress& address);
  void stopDnsWildcardRouter();

 protected:
  virtual bool run(Op op, const InterfaceConfig& config) {
    Q_UNUSED(op);
    Q_UNUSED(config);
    return true;
  }
  virtual bool supportServerSwitching(const InterfaceConfig& config) const;
  virtual bool switchServer(const InterfaceConfig& config);
  virtual WireguardUtils* wgutils() const = 0;
  virtual bool supportIPUtils() const { return false; }
  virtual IPUtils* iputils() { return nullptr; }
  virtual DnsUtils* dnsutils() { return nullptr; }

  virtual bool allowBypassAddress(const IPAddress& prefix) {
    Q_UNUSED(prefix);
    return true;
  }
  virtual bool revokeBypassAddress(const IPAddress& prefix) {
    Q_UNUSED(prefix);
    return true;
  }

  // Withdraws every bypass permit at once. Returns false when the platform has
  // no bulk operation, and the caller then revokes address by address.
  virtual bool revokeAllBypassAddresses() { return false; }

  // Puts back the permits of addresses that are still routed, after the
  // firewall may have been rebuilt underneath them. Only needed where a
  // rebuild drops them; must be idempotent.
  virtual void reassertBypassPermits(const QList<IPAddress>& addresses,
                                     const QHostAddress& resolver) {
    Q_UNUSED(addresses);
    Q_UNUSED(resolver);
  }

  // Whether an exclusion route can be installed for an IPv6 address.
  virtual bool supportsIPv6ExclusionRoutes() const { return true; }

  virtual bool allowResolverEndpoint(const QHostAddress& address) {
    Q_UNUSED(address);
    return true;
  }
  virtual bool revokeResolverEndpoint(const QHostAddress& address) {
    Q_UNUSED(address);
    return true;
  }

  static bool parseStringList(const QJsonObject& obj, const QString& name,
                              QStringList& list);

  void checkHandshake();

  class ConnectionState {
   public:
    ConnectionState(){};
    ConnectionState(const InterfaceConfig& config) { m_config = config; }
    QDateTime m_date;
    InterfaceConfig m_config;
  };
  QMap<InterfaceConfig::HopType, ConnectionState> m_connections;
  QHash<IPAddress, int> m_excludedAddrSet;
  QTimer m_handshakeTimer;
  DnsWildcardRouter* m_dnsWildcardRouter = nullptr;
  WildcardRouteTable m_wildcardRoutes;
  QStringList m_wildcardPatterns;
};

#endif  // DAEMON_H
