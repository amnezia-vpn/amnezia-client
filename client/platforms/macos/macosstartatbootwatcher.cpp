/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosstartatbootwatcher.h"
#include "leakdetector.h"
#include "logger.h"
#include "macosutils.h"

namespace {
Logger logger(LOG_MACOS, "MacOSStartAtBootWatcher");
}

MacOSStartAtBootWatcher::MacOSStartAtBootWatcher(bool startAtBoot) {
  MVPN_COUNT_CTOR(MacOSStartAtBootWatcher);

  logger.debug() << "StartAtBoot watcher";
  MacOSUtils::enableLoginItem(startAtBoot);
}

MacOSStartAtBootWatcher::~MacOSStartAtBootWatcher() {
  MVPN_COUNT_DTOR(MacOSStartAtBootWatcher);
}

void MacOSStartAtBootWatcher::startAtBootChanged(bool startAtBoot) {
  logger.debug() << "StartAtBoot changed:" << startAtBoot;
  MacOSUtils::enableLoginItem(startAtBoot);
}
