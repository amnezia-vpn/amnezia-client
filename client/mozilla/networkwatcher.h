/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NETWORKWATCHER_H
#define NETWORKWATCHER_H

#include <QElapsedTimer>
#include <QMap>
#include <QObject>

class NetworkWatcherImpl;

// This class watches for network changes to detect unsecured wifi.
class NetworkWatcher final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(NetworkWatcher)

 public:
  NetworkWatcher();
  ~NetworkWatcher();

  void initialize();

  // public for the inspector.
  void unsecuredNetwork(const QString& networkName, const QString& networkId);

  QString getCurrentTransport();

 signals:
  void networkChange();

 private:
  void settingsChanged();

 // void notificationClicked(NotificationHandler::Message message);

 private:
  bool m_active = false;
  bool m_reportUnsecuredNetwork = false;

  // Platform-specific implementation.
  NetworkWatcherImpl* m_impl = nullptr;

  QMap<QString, QElapsedTimer> m_networks;

  // This is used to connect NotificationHandler lazily.
  bool m_firstNotification = true;
};

#endif  // NETWORKWATCHER_H
