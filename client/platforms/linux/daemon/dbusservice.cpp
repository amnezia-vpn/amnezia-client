/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dbusservice.h"
#include "dbus_adaptor.h"
#include "leakdetector.h"
#include "logger.h"
#include "loghandler.h"
#include "polkithelper.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
Logger logger(LOG_LINUX, "DBusService");
}

constexpr const char* APP_STATE_ACTIVE = "active";
constexpr const char* APP_STATE_EXCLUDED = "excluded";
constexpr const char* APP_STATE_BLOCKED = "blocked";

DBusService::DBusService(QObject* parent) : Daemon(parent) {
  MVPN_COUNT_CTOR(DBusService);

  m_wgutils = new WireguardUtilsLinux(this);
  m_apptracker = new AppTracker(this);
  m_pidtracker = new PidTracker(this);

  connect(m_apptracker, SIGNAL(appLaunched(const QString&, int)), this,
          SLOT(appLaunched(const QString&, int)));
  connect(m_pidtracker, SIGNAL(terminated(const QString&, int)), this,
          SLOT(appTerminated(const QString&, int)));

  if (!removeInterfaceIfExists()) {
    qFatal("Interface `%s` exists and cannot be removed. Cannot proceed!",
           WG_INTERFACE);
  }
}

DBusService::~DBusService() { MVPN_COUNT_DTOR(DBusService); }

IPUtils* DBusService::iputils() {
  if (!m_iputils) {
    m_iputils = new IPUtilsLinux(this);
  }
  return m_iputils;
}

DnsUtils* DBusService::dnsutils() {
  if (!m_dnsutils) {
    m_dnsutils = new DnsUtilsLinux(this);
  }
  return m_dnsutils;
}

void DBusService::setAdaptor(DbusAdaptor* adaptor) {
  Q_ASSERT(!m_adaptor);
  m_adaptor = adaptor;
}

bool DBusService::removeInterfaceIfExists() {
  if (m_wgutils->interfaceExists()) {
    logger.warning() << "Device already exists. Let's remove it.";
    if (!m_wgutils->deleteInterface()) {
      logger.error() << "Failed to remove the device.";
      return false;
    }
  }
  return true;
}

QString DBusService::version() {
  logger.debug() << "Version request";
  return PROTOCOL_VERSION;
}

bool DBusService::activate(const QString& jsonConfig) {
  logger.debug() << "Activate";

  if (!PolkitHelper::instance()->checkAuthorization(
          "org.mozilla.vpn.activate")) {
    logger.error() << "Polkit rejected";
    return false;
  }

  QJsonDocument json = QJsonDocument::fromJson(jsonConfig.toLocal8Bit());
  if (!json.isObject()) {
    logger.error() << "Invalid input";
    return false;
  }

  QJsonObject obj = json.object();

  InterfaceConfig config;
  if (!parseConfig(obj, config)) {
    logger.error() << "Invalid configuration";
    return false;
  }

  if (obj.contains("vpnDisabledApps")) {
    QJsonArray disabledApps = obj["vpnDisabledApps"].toArray();
    for (const QJsonValue& app : disabledApps) {
      firewallApp(app.toString(), APP_STATE_EXCLUDED);
    }
  }

  return Daemon::activate(config);
}

bool DBusService::deactivate(bool emitSignals) {
  logger.debug() << "Deactivate";
  firewallClear();
  return Daemon::deactivate(emitSignals);
}

QString DBusService::status() { return QString(getStatus()); }

QByteArray DBusService::getStatus() {
  logger.debug() << "Status request";
  QJsonObject json;

  if (!m_connections.contains(0)) {
    json.insert("status", QJsonValue(false));
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
  }

  const InterfaceConfig& config = m_connections.value(0).m_config;
  if (!m_wgutils->interfaceExists()) {
    logger.error() << "Unable to get device";
    json.insert("status", QJsonValue(false));
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
  }

  json.insert("status", QJsonValue(true));
  json.insert("serverIpv4Gateway", QJsonValue(config.m_serverIpv4Gateway));
  json.insert("deviceIpv4Address", QJsonValue(config.m_deviceIpv4Address));
  WireguardUtilsLinux::peerStatus status =
      m_wgutils->getPeerStatus(config.m_serverPublicKey);
  json.insert("txBytes", QJsonValue(status.txBytes));
  json.insert("rxBytes", QJsonValue(status.rxBytes));

  return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString DBusService::getLogs() {
  logger.debug() << "Log request";
  return Daemon::logs();
}

void DBusService::appLaunched(const QString& name, int rootpid) {
  logger.debug() << "tracking:" << name << "PID:" << rootpid;
  ProcessGroup* group = m_pidtracker->track(name, rootpid);
  if (m_firewallApps.contains(name)) {
    group->state = m_firewallApps[name];
    group->moveToCgroup(getAppStateCgroup(group->state));
  }
}

void DBusService::appTerminated(const QString& name, int rootpid) {
  logger.debug() << "terminate:" << name << "PID:" << rootpid;
}

/* Get the list of running applications that the firewall knows about. */
QString DBusService::runningApps() {
  QJsonArray result;
  for (auto i = m_pidtracker->begin(); i != m_pidtracker->end(); i++) {
    const ProcessGroup* group = *i;
    QJsonObject appObject;
    QJsonArray pidList;
    appObject.insert("name", QJsonValue(group->name));
    appObject.insert("rootpid", QJsonValue(group->rootpid));
    appObject.insert("state", QJsonValue(group->state));

    for (auto pid : group->kthreads.keys()) {
      pidList.append(QJsonValue(pid));
    }

    appObject.insert("pids", pidList);
    result.append(appObject);
  }

  return QJsonDocument(result).toJson(QJsonDocument::Compact);
}

/* Update the firewall for running applications matching the application ID. */
bool DBusService::firewallApp(const QString& appName, const QString& state) {
  logger.debug() << "Setting" << appName << "to firewall state" << state;
  m_firewallApps[appName] = state;
  QString cgroup = getAppStateCgroup(state);

  /* Change matching applications' state to excluded */
  for (auto i = m_pidtracker->begin(); i != m_pidtracker->end(); i++) {
    ProcessGroup* group = *i;
    if (group->name != appName) {
      continue;
    }
    group->state = state;
    group->moveToCgroup(cgroup);
  }

  return true;
}

/* Update the firewall for the application matching the desired PID. */
bool DBusService::firewallPid(int rootpid, const QString& state) {
  ProcessGroup* group = m_pidtracker->group(rootpid);
  if (!group) {
    return false;
  }

  group->state = state;
  group->moveToCgroup(getAppStateCgroup(group->state));

  logger.debug() << "Setting" << group->name << "PID:" << rootpid
                 << "to firewall state" << state;
  return true;
}

/* Clear the firewall and return all applications to the active state */
bool DBusService::firewallClear() {
  const QString cgroup = getAppStateCgroup(APP_STATE_ACTIVE);

  m_firewallApps.clear();
  for (auto i = m_pidtracker->begin(); i != m_pidtracker->end(); i++) {
    ProcessGroup* group = *i;
    if (group->state == APP_STATE_ACTIVE) {
      continue;
    }

    group->state = APP_STATE_ACTIVE;
    group->moveToCgroup(cgroup);

    logger.debug() << "Setting" << group->name << "PID:" << group->rootpid
                   << "to firewall state" << group->state;
  }
  return true;
}

QString DBusService::getAppStateCgroup(const QString& state) {
  if (state == APP_STATE_EXCLUDED) {
    return m_wgutils->getExcludeCgroup();
  }
  if (state == APP_STATE_BLOCKED) {
    return m_wgutils->getBlockCgroup();
  }
  return m_wgutils->getDefaultCgroup();
}
