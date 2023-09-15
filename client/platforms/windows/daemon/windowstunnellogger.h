/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSTUNNELLOGGER_H
#define WINDOWSTUNNELLOGGER_H

#include <QFile>
#include <QObject>
#include <QTimer>

class WindowsTunnelLogger final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(WindowsTunnelLogger)

 public:
  WindowsTunnelLogger(const QString& filename, QObject* parent = nullptr);
  ~WindowsTunnelLogger();

 private slots:
  void timeout();

 private:
  bool openLogData();
  void process(int index);
  int nextIndex();

 private:
  QTimer m_timer;
  QFile m_logfile;
  uchar* m_logdata = nullptr;
  int m_logindex = -1;
  quint64 m_startTime = 0;
};

#endif  // WINDOWSTUNNELLOGGER_H
