/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosdaemon.h"

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

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("MacOSDaemon");
MacOSDaemon* s_daemon = nullptr;
}  // namespace

MacOSDaemon::MacOSDaemon() : Daemon(nullptr) {
  MZ_COUNT_CTOR(MacOSDaemon);

  logger.debug() << "Daemon created";

  m_wgutils = new WireguardUtilsMacos(this);
  m_dnsutils = new DnsUtilsMacos(this);
  m_iputils = new IPUtilsMacos(this);

  Q_ASSERT(s_daemon == nullptr);
  s_daemon = this;
}

MacOSDaemon::~MacOSDaemon() {
  MZ_COUNT_DTOR(MacOSDaemon);

  logger.debug() << "Daemon released";

  Q_ASSERT(s_daemon == this);
  s_daemon = nullptr;
}

// static
MacOSDaemon* MacOSDaemon::instance() {
  Q_ASSERT(s_daemon);
  return s_daemon;
}
