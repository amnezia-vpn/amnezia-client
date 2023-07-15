/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DUMMYPINGSENDER_H
#define DUMMYPINGSENDER_H

#include "pingsender.h"

class DummyPingSender final : public PingSender {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DummyPingSender)

 public:
  DummyPingSender(const QHostAddress& source, QObject* parent = nullptr);
  ~DummyPingSender();

  void sendPing(const QHostAddress& dest, quint16 sequence) override;
};

#endif  // DUMMYPINGSENDER_H
