/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXNETWORKWATCHERWORKER_H
#define LINUXNETWORKWATCHERWORKER_H

#include <QMap>
#include <QObject>
#include <QVariant>

class QThread;

class LinuxNetworkWatcherWorker final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LinuxNetworkWatcherWorker)

 public:
  explicit LinuxNetworkWatcherWorker(QThread* thread);
  ~LinuxNetworkWatcherWorker();

  void checkDevices();

 signals:
  void unsecuredNetwork(const QString& networkName, const QString& networkId);

 public slots:
  void initialize();

 private slots:
  void propertyChanged(QString interface, QVariantMap properties,
                       QStringList list);

 private:
  // We collect the list of DBus wifi network device paths during the
  // initialization. When a property of them changes, we check if the access
  // point is active and unsecure.
  QStringList m_devicePaths;
};

#endif  // LINUXNETWORKWATCHERWORKER_H
