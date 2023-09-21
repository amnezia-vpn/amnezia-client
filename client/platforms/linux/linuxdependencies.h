/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXDEPENDENCIES_H
#define LINUXDEPENDENCIES_H

#include <QObject>

class LinuxDependencies final {
 public:
  static bool checkDependencies();
  static QString findCgroupPath(const QString& type);
  static QString findCgroup2Path();
  static QString gnomeShellVersion();
  static QString kdeFrameworkVersion();

 private:
  LinuxDependencies() = default;
  ~LinuxDependencies() = default;

  Q_DISABLE_COPY(LinuxDependencies)
};

#endif  // LINUXDEPENDENCIES_H
