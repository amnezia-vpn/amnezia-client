/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BACKENDLOGSOBSERVER_H
#define BACKENDLOGSOBSERVER_H

#include <functional>
#include <QObject>

class QDBusPendingCallWatcher;

class BackendLogsObserver final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(BackendLogsObserver)

 public:
  BackendLogsObserver(QObject* parent,
                      std::function<void(const QString&)>&& callback);
  ~BackendLogsObserver();

 public slots:
  void completed(QDBusPendingCallWatcher* call);

 private:
  std::function<void(const QString&)> m_callback;
};

#endif  // BACKENDLOGSOBSERVER_H
