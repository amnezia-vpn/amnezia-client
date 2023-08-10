/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "leakdetector.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QTextStream>

#ifdef MZ_DEBUG
static QMutex s_leakDetector;

QHash<QString, QHash<void*, uint32_t>> s_leaks;
#endif

LeakDetector::LeakDetector() {
#ifndef MZ_DEBUG
  qFatal("LeakDetector _must_ be created in debug builds only!");
#endif
}

LeakDetector::~LeakDetector() {
#ifdef MZ_DEBUG
  QTextStream out(stderr);

  out << "== MZ  - Leak report ===================" << Qt::endl;

  bool hasLeaks = false;
  for (auto i = s_leaks.begin(); i != s_leaks.end(); ++i) {
    QString className = i.key();

    if (i->size() == 0) {
      continue;
    }

    hasLeaks = true;
    out << className << Qt::endl;

    for (auto l = i->begin(); l != i->end(); ++l) {
      out << "  - ptr: " << l.key() << " size:" << l.value() << Qt::endl;
    }
  }

  if (!hasLeaks) {
    out << "No leaks detected." << Qt::endl;
  }
#endif
}

#ifdef MZ_DEBUG
void LeakDetector::logCtor(void* ptr, const char* typeName, uint32_t size) {
  QMutexLocker lock(&s_leakDetector);

  QString type(typeName);
  if (!s_leaks.contains(type)) {
    s_leaks.insert(type, QHash<void*, uint32_t>());
  }

  s_leaks[type].insert(ptr, size);
}

void LeakDetector::logDtor(void* ptr, const char* typeName, uint32_t size) {
  QMutexLocker lock(&s_leakDetector);

  QString type(typeName);
  Q_ASSERT(s_leaks.contains(type));

  QHash<void*, uint32_t>& leak = s_leaks[type];
  Q_ASSERT(leak.contains(ptr));
  Q_ASSERT(leak[ptr] == size);
  leak.remove(ptr);
}
#endif
