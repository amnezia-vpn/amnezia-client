/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DNSPINGSENDER_H
#define DNSPINGSENDER_H

#include <QUdpSocket>

#include "pingsender.h"

class DnsPingSender final : public PingSender {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DnsPingSender)

 public:
  DnsPingSender(const QHostAddress& source, QObject* parent = nullptr);
  ~DnsPingSender();

  void sendPing(const QHostAddress& dest, quint16 sequence) override;

  void start();
  void stop() { m_socket.close(); }

 private:
  void readData();

 private:
  QUdpSocket m_socket;
  QHostAddress m_source;
};

#endif  // DNSPINGSENDER_H
