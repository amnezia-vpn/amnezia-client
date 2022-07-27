/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LEAKDETECTOR_H
#define LEAKDETECTOR_H

#include <QObject>

#ifdef MVPN_DEBUG
#  define MVPN_COUNT_CTOR(_type)                                 \
    do {                                                         \
      static_assert(std::is_class<_type>(),                      \
                    "Token '" #_type "' is not a class type.");  \
      LeakDetector::logCtor((void*)this, #_type, sizeof(*this)); \
    } while (0)

#  define MVPN_COUNT_DTOR(_type)                                 \
    do {                                                         \
      static_assert(std::is_class<_type>(),                      \
                    "Token '" #_type "' is not a class type.");  \
      LeakDetector::logDtor((void*)this, #_type, sizeof(*this)); \
    } while (0)

#else
#  define MVPN_COUNT_CTOR(_type)
#  define MVPN_COUNT_DTOR(_type)
#endif

class LeakDetector {
 public:
  LeakDetector();
  ~LeakDetector();

#ifdef MVPN_DEBUG
  static void logCtor(void* ptr, const char* typeName, uint32_t size);
  static void logDtor(void* ptr, const char* typeName, uint32_t size);
#endif
};

#endif  // LEAKDETECTOR_H
