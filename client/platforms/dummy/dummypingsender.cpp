/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dummypingsender.h"

#include "leakdetector.h"
#include "logger.h"

namespace {
Logger logger("DummyPingSender");
}

DummyPingSender::DummyPingSender(const QHostAddress& source, QObject* parent)
    : PingSender(parent) {
  MZ_COUNT_CTOR(DummyPingSender);
  Q_UNUSED(source);
}

DummyPingSender::~DummyPingSender() { MZ_COUNT_DTOR(DummyPingSender); }

void DummyPingSender::sendPing(const QHostAddress& dest, quint16 sequence) {
  logger.debug() << "Dummy ping to:" << dest.toString();
  emit recvPing(sequence);
}
