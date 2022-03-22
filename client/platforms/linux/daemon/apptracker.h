/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef APPTRACKER_H
#define APPTRACKER_H

#include <QDBusPendingCallWatcher>
#include <QDBusObjectPath>

class AppTracker final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(AppTracker)

 public:
  explicit AppTracker(QObject* parent);
  ~AppTracker();

 signals:
  void appLaunched(const QString& name, int rootpid);

 private slots:
  void userListCompleted(QDBusPendingCallWatcher* call);
  void userCreated(uint uid, const QDBusObjectPath& path);
  void userRemoved(uint uid, const QDBusObjectPath& path);
  void gtkLaunchEvent(const QByteArray& appid, const QString& display,
                      qlonglong pid, const QStringList& uris,
                      const QVariantMap& extra);
};

#endif  // APPTRACKER_H
