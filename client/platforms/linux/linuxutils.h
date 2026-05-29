/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXUTILS_H
#define LINUXUTILS_H

#include <functional>

class LinuxUtils final {
 public:
  static bool isDarkTheme();
  static void installThemeChangeObserver(std::function<void()> callback);

 private:
  LinuxUtils() = default;
};

#endif  // LINUXUTILS_H
