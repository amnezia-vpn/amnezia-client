/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LINUXPINGSENDER_H
#define LINUXPINGSENDER_H

#include <QObject>

#include "../../mozilla/pingsender.h"

class QSocketNotifier;

class LinuxPingSender final : public PingSender {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LinuxPingSender)

 public:
  LinuxPingSender(const QHostAddress& source, QObject* parent = nullptr);
  ~LinuxPingSender();

  bool isValid() override { return (m_socket >= 0); };

  void sendPing(const QHostAddress& dest, quint16 sequence) override;

 private:
  int createSocket();

 private slots:
  void rawSocketReady();
  void icmpSocketReady();

 private:
  QSocketNotifier* m_notifier = nullptr;
  int m_socket = -1;
  quint16 m_ident = 0;
};

#endif  // LINUXPINGSENDER_H
