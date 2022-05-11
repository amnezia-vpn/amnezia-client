/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dbusclient.h"
#include "ipaddressrange.h"
#include "leakdetector.h"
#include "logger.h"
#include "models/device.h"
#include "models/keys.h"
#include "models/server.h"
#include "mozillavpn.h"
#include "settingsholder.h"

#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

constexpr const char* DBUS_SERVICE = "org.mozilla.vpn.dbus";
constexpr const char* DBUS_PATH = "/";

namespace {
Logger logger(LOG_LINUX, "DBusClient");
}

DBusClient::DBusClient(QObject* parent) : QObject(parent) {
  MVPN_COUNT_CTOR(DBusClient);

  m_dbus = new OrgMozillaVpnDbusInterface(DBUS_SERVICE, DBUS_PATH,
                                          QDBusConnection::systemBus(), this);

  connect(m_dbus, &OrgMozillaVpnDbusInterface::connected, this,
          &DBusClient::connected);
  connect(m_dbus, &OrgMozillaVpnDbusInterface::disconnected, this,
          &DBusClient::disconnected);
}

DBusClient::~DBusClient() { MVPN_COUNT_DTOR(DBusClient); }

QDBusPendingCallWatcher* DBusClient::version() {
  logger.debug() << "Version via DBus";
  QDBusPendingReply<QString> reply = m_dbus->version();
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                   &QDBusPendingCallWatcher::deleteLater);
  return watcher;
}

QDBusPendingCallWatcher* DBusClient::activate(
    const Server& server, const Device* device, const Keys* keys, int hopindex,
    const QList<IPAddressRange>& allowedIPAddressRanges,
    const QStringList& vpnDisabledApps, const QHostAddress& dnsServer) {
  QJsonObject json;
  json.insert("privateKey", QJsonValue(keys->privateKey()));
  json.insert("deviceIpv4Address", QJsonValue(device->ipv4Address()));
  json.insert("deviceIpv6Address", QJsonValue(device->ipv6Address()));
  json.insert("serverIpv4Gateway", QJsonValue(server.ipv4Gateway()));
  json.insert("serverIpv6Gateway", QJsonValue(server.ipv6Gateway()));
  json.insert("serverPublicKey", QJsonValue(server.publicKey()));
  json.insert("serverIpv4AddrIn", QJsonValue(server.ipv4AddrIn()));
  json.insert("serverIpv6AddrIn", QJsonValue(server.ipv6AddrIn()));
  json.insert("serverPort", QJsonValue((double)server.choosePort()));
  json.insert("dnsServer", QJsonValue(dnsServer.toString()));
  json.insert("hopindex", QJsonValue((double)hopindex));

  QJsonArray allowedIPAddesses;
  for (const IPAddressRange& i : allowedIPAddressRanges) {
    QJsonObject range;
    range.insert("address", QJsonValue(i.ipAddress()));
    range.insert("range", QJsonValue((double)i.range()));
    range.insert("isIpv6", QJsonValue(i.type() == IPAddressRange::IPv6));
    allowedIPAddesses.append(range);
  };
  json.insert("allowedIPAddressRanges", allowedIPAddesses);

  QJsonArray disabledApps;
  for (const QString& i : vpnDisabledApps) {
    disabledApps.append(QJsonValue(i));
    logger.debug() << "Disabling:" << i;
  }
  json.insert("vpnDisabledApps", disabledApps);

  logger.debug() << "Activate via DBus";
  QDBusPendingReply<bool> reply =
      m_dbus->activate(QJsonDocument(json).toJson(QJsonDocument::Compact));
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                   &QDBusPendingCallWatcher::deleteLater);
  return watcher;
}

QDBusPendingCallWatcher* DBusClient::deactivate() {
  logger.debug() << "Deactivate via DBus";
  QDBusPendingReply<bool> reply = m_dbus->deactivate();
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                   &QDBusPendingCallWatcher::deleteLater);
  return watcher;
}

QDBusPendingCallWatcher* DBusClient::status() {
  logger.debug() << "Status via DBus";
  QDBusPendingReply<QString> reply = m_dbus->status();
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                   &QDBusPendingCallWatcher::deleteLater);
  return watcher;
}

QDBusPendingCallWatcher* DBusClient::getLogs() {
  logger.debug() << "Get logs via DBus";
  QDBusPendingReply<QString> reply = m_dbus->getLogs();
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                   &QDBusPendingCallWatcher::deleteLater);
  return watcher;
}

QDBusPendingCallWatcher* DBusClient::cleanupLogs() {
  logger.debug() << "Cleanup logs via DBus";
  QDBusPendingReply<QString> reply = m_dbus->cleanupLogs();
  QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
  QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                   &QDBusPendingCallWatcher::deleteLater);
  return watcher;
}
