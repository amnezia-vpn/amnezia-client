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
#include "interfaceconfig.h"
#include "iputils.h"
#include "wireguardutils.h"

class Daemon : public QObject {
  Q_OBJECT

 public:
  enum Op {
    Up,
    Down,
  };

  explicit Daemon(QObject* parent);
  ~Daemon();

  static Daemon* instance();

  static bool parseConfig(const QJsonObject& obj, InterfaceConfig& config);

  bool activate(const QString& ifname, const InterfaceConfig& config);
  bool setPrimary(const QString& ifname, const InterfaceConfig& config);
  bool deactivateTunnel(const QString& ifname);
  virtual bool deactivate(bool emitSignals = true);
  virtual QJsonObject getStatus();

  bool addExclusionRoute(const QString &addr);
  bool delExclusionRoute(const QString &addr);
  bool addAllowedIp(const QString &ifname, const QString &prefix);
  bool delAllowedIp(const QString &ifname, const QString &prefix);
  bool setTunnelResolvers(const QString &ifname, const QStringList &resolvers);
  bool restoreTunnelResolvers();

  const QString& primaryIfname() const { return m_primaryIfname; }
  WireguardUtils* wgutilsFor(const QString& ifname) const { return m_tunnels.value(ifname); }

  // Callback before any Activating measure is done
  virtual void prepareActivation(const InterfaceConfig& config, int inetAdapterIndex = 0) {
      Q_UNUSED(config)  };
  virtual void activateSplitTunnel(const InterfaceConfig& config, int vpnAdapterIndex = 0) {
      Q_UNUSED(config)  };

  QString logs();
  void cleanLogs();

 signals:
  void tunnelConnected(const QString& ifname, const QString& pubkey);
  void tunnelHandshakeFailed(const QString& ifname);
  void disconnected();
  void backendFailure(DaemonError reason = DaemonError::ERROR_FATAL);

 private:
  void checkActivations();
  WireguardUtils* primaryWgutils() const { return m_tunnels.value(m_primaryIfname); }
  QTimer m_activationTimer;

 protected:
  virtual bool run(Op op, const InterfaceConfig& config) {
    Q_UNUSED(op);
    Q_UNUSED(config);
    return true;
  }
  virtual WireguardUtils* createWgUtils() = 0;

  QMap<QString, WireguardUtils*> m_tunnels;
  QString m_primaryIfname;

  virtual bool supportIPUtils() const { return false; }
  virtual IPUtils* iputils() { return nullptr; }
  virtual DnsUtils* dnsutils() { return nullptr; }

  static bool parseStringList(const QJsonObject& obj, const QString& name,
                              QStringList& list);

  class ConnectionState {
   public:
    ConnectionState(){};
    ConnectionState(const InterfaceConfig& config) { m_config = config; }
    QDateTime m_date;
    QDateTime m_deadline;
    InterfaceConfig m_config;
  };
  QMap<QString, ConnectionState> m_connections;
  QHash<IPAddress, int> m_excludedAddrSet;
};

#endif  // DAEMON_H
