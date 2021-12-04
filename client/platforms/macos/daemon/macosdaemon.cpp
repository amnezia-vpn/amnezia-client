/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosdaemon.h"
#include "leakdetector.h"
#include "logger.h"
#include "wgquickprocess.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalSocket>
#include <QProcess>
#include <QSettings>
#include <QTextStream>
#include <QtGlobal>

namespace {
Logger logger(LOG_MACOS, "MacOSDaemon");
MacOSDaemon* s_daemon = nullptr;
}  // namespace

MacOSDaemon::MacOSDaemon() : Daemon(nullptr) {
  MVPN_COUNT_CTOR(MacOSDaemon);

  logger.debug() << "Daemon created";

  m_wgutils = new WireguardUtilsMacos(this);
  m_dnsutils = new DnsUtilsMacos(this);
  m_iputils = new IPUtilsMacos(this);

  Q_ASSERT(s_daemon == nullptr);
  s_daemon = this;
}

MacOSDaemon::~MacOSDaemon() {
  MVPN_COUNT_DTOR(MacOSDaemon);

  logger.debug() << "Daemon released";

  Q_ASSERT(s_daemon == this);
  s_daemon = nullptr;
}

// static
MacOSDaemon* MacOSDaemon::instance() {
  Q_ASSERT(s_daemon);
  return s_daemon;
}

QByteArray MacOSDaemon::getStatus() {
  logger.debug() << "Status request";

  bool connected = m_connections.contains(0);
  QJsonObject obj;
  obj.insert("type", "status");
  obj.insert("connected", connected);

  if (connected) {
    const ConnectionState& state = m_connections.value(0).m_config;
    WireguardUtils::peerStatus status =
        m_wgutils->getPeerStatus(state.m_config.m_serverPublicKey);
    obj.insert("serverIpv4Gateway", state.m_config.m_serverIpv4Gateway);
    obj.insert("deviceIpv4Address", state.m_config.m_deviceIpv4Address);
    obj.insert("date", state.m_date.toString());
    obj.insert("txBytes", QJsonValue(status.txBytes));
    obj.insert("rxBytes", QJsonValue(status.rxBytes));
  }

  return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
