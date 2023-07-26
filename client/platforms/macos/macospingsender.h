/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSPINGSENDER_H
#define MACOSPINGSENDER_H

#include "pingsender.h"

class QSocketNotifier;

class MacOSPingSender final : public PingSender {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MacOSPingSender)

 public:
  MacOSPingSender(const QHostAddress& source, QObject* parent = nullptr);
  ~MacOSPingSender();

  void sendPing(const QHostAddress& dest, quint16 sequence) override;

 private slots:
  void socketReady();

 private:
  QSocketNotifier* m_notifier = nullptr;
  int m_socket = -1;
};

#endif  // MACOSPINGSENDER_H
