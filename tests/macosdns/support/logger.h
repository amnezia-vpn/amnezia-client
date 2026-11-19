#pragma once
#include <QDebug>
// Keep the production DNS class independent of application logging/IPC in tests.
class Logger {
 public:
  explicit Logger(const char*) {}
  QDebug debug() { return qDebug(); }
  QDebug error() { return qWarning(); }
  QDebug warning() { return qWarning(); }
};
