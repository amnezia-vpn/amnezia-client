#include "swift_loggers.h"

#include <QDebug>

void logError(std::string message) {
  qCritical() << message;
}

void logDebug(std::string message) {
  qDebug() << message;
}
