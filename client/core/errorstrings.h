#ifndef ERRORSTRINGS_H
#define ERRORSTRINGS_H

#include <QDebug>

#include "defs.h"

using namespace amnezia;

QString errorString(ErrorCode code);

QDebug operator<<(QDebug debug, const ErrorCode &e);

#endif // ERRORSTRINGS_H
