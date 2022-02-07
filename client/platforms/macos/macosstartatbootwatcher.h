/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSSTARTATBOOTWATCHER_H
#define MACOSSTARTATBOOTWATCHER_H

#include <QObject>

class MacOSStartAtBootWatcher final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MacOSStartAtBootWatcher)

 public:
  explicit MacOSStartAtBootWatcher(bool startAtBoot);
  ~MacOSStartAtBootWatcher();

 public slots:
  void startAtBootChanged(bool value);
};

#endif  // MACOSSTARTATBOOTWATCHER_H
