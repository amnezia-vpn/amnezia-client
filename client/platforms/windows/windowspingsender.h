/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSPINGSENDER_H
#define WINDOWSPINGSENDER_H

#include <QMap>
#include <QWinEventNotifier>

#include "pingsender.h"

struct WindowsPingSenderPrivate;

class WindowsPingSender final : public PingSender {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(WindowsPingSender)

 public:
  WindowsPingSender(const QHostAddress& source, QObject* parent = nullptr);
  ~WindowsPingSender();

  void sendPing(const QHostAddress& destination, quint16 sequence) override;

 private slots:
  void pingEventReady();

 private:
  QHostAddress m_source;
  QWinEventNotifier* m_notifier = nullptr;
  struct WindowsPingSenderPrivate* m_private = nullptr;
};

#endif  // WINDOWSPINGSENDER_H
