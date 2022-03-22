/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DBUSSERVICE_H
#define DBUSSERVICE_H

#include "daemon/daemon.h"
#include "apptracker.h"
#include "iputilslinux.h"
#include "dnsutilslinux.h"
#include "pidtracker.h"
#include "wireguardutilslinux.h"

class DbusAdaptor;

class DBusService final : public Daemon {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DBusService)
  Q_CLASSINFO("D-Bus Interface", "org.mozilla.vpn.dbus")

 public:
  DBusService(QObject* parent);
  ~DBusService();

  void setAdaptor(DbusAdaptor* adaptor);

  using Daemon::activate;

 public slots:
  bool activate(const QString& jsonConfig);

  bool deactivate(bool emitSignals = true) override;
  QString status();

  QString version();
  QString getLogs();

  QString runningApps();
  bool firewallApp(const QString& appName, const QString& state);
  bool firewallPid(int rootpid, const QString& state);
  bool firewallClear();

 protected:
  WireguardUtils* wgutils() const override { return m_wgutils; }
  bool supportIPUtils() const override { return true; }
  IPUtils* iputils() override;
  bool supportDnsUtils() const override { return true; }
  DnsUtils* dnsutils() override;

  QByteArray getStatus() override;

 private:
  bool removeInterfaceIfExists();
  QString getAppStateCgroup(const QString& state);

 private slots:
  void appLaunched(const QString& name, int rootpid);
  void appTerminated(const QString& name, int rootpid);

 private:
  DbusAdaptor* m_adaptor = nullptr;
  WireguardUtilsLinux* m_wgutils = nullptr;
  IPUtilsLinux* m_iputils = nullptr;
  DnsUtilsLinux* m_dnsutils = nullptr;

  AppTracker* m_apptracker = nullptr;
  PidTracker* m_pidtracker = nullptr;
  QMap<QString, QString> m_firewallApps;
};

#endif  // DBUSSERVICE_H
