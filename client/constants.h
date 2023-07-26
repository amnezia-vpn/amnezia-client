/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "version.h"

#include <stdint.h>
#include <QObject>

namespace Constants {

inline QString versionString() { return QStringLiteral(APP_VERSION); }

constexpr const char* PLATFORM_NAME =
#if defined(MZ_IOS)
    "ios"
#elif defined(MZ_MACOS)
    "macos"
#elif defined(MZ_LINUX)
    "linux"
#elif defined(MZ_ANDROID)
    "android"
#elif defined(MZ_WINDOWS)
    "windows"
#elif defined(UNIT_TEST) || defined(MZ_DUMMY)
    "dummy"
#else
#  error "Unsupported platform"
#endif
    ;
}
#endif  // CONSTANTS_H
