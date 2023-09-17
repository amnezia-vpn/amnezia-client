/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxdaemon.h"

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
Logger logger("LinuxDaemon");
LinuxDaemon* s_daemon = nullptr;
}  // namespace

LinuxDaemon::LinuxDaemon() : Daemon(nullptr) {
    MZ_COUNT_CTOR(LinuxDaemon);

    logger.debug() << "Daemon created";

    m_wgutils = new WireguardUtilsLinux(this);
    m_dnsutils = new DnsUtilsLinux(this);
    m_iputils = new IPUtilsLinux(this);

    Q_ASSERT(s_daemon == nullptr);
    s_daemon = this;
}

LinuxDaemon::~LinuxDaemon() {
    MZ_COUNT_DTOR(LinuxDaemon);

    logger.debug() << "Daemon released";

    Q_ASSERT(s_daemon == this);
    s_daemon = nullptr;
}

// static
LinuxDaemon* LinuxDaemon::instance() {
    Q_ASSERT(s_daemon);
    return s_daemon;
}
