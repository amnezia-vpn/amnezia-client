/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "backendlogsobserver.h"
#include "leakdetector.h"
#include "logger.h"

#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

namespace {
Logger logger({LOG_LINUX, LOG_CONTROLLER}, "BackendLogsObserver");
}

BackendLogsObserver::BackendLogsObserver(
    QObject* parent, std::function<void(const QString&)>&& callback)
    : QObject(parent), m_callback(std::move(callback)) {
  MVPN_COUNT_CTOR(BackendLogsObserver);
}

BackendLogsObserver::~BackendLogsObserver() {
  MVPN_COUNT_DTOR(BackendLogsObserver);
}

void BackendLogsObserver::completed(QDBusPendingCallWatcher* call) {
  QDBusPendingReply<QString> reply = *call;
  if (reply.isError()) {
    logger.error() << "Error received from the DBus service";
    m_callback("Failed to retrieve logs from the mozillavpn linuxdaemon.");
    return;
  }

  QString status = reply.argumentAt<0>();
  m_callback(status);
}
