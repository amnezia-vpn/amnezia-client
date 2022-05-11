/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXAPPLISTPROVIDER_H
#define LINUXAPPLISTPROVIDER_H

#include <applistprovider.h>
#include <QObject>
#include <QProcess>

class LinuxAppListProvider final : public AppListProvider {
  Q_OBJECT
 public:
  explicit LinuxAppListProvider(QObject* parent);
  ~LinuxAppListProvider();
  void getApplicationList() override;

 private:
  void fetchEntries(const QString& dataDir, QMap<QString, QString>& map);
};

#endif  // LINUXAPPLISTPROVIDER_H
