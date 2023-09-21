/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PINGHELPER_H
#define PINGHELPER_H

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QVector>

class PingSender;

class PingHelper final : public QObject {
 private:
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(PingHelper)

 public:
  PingHelper();
  ~PingHelper();

  void start(const QString& serverIpv4Gateway,
             const QString& deviceIpv4Address);

  void stop();
  uint latency() const;
  uint stddev() const;
  uint maximum() const;
  double loss() const;

 signals:
  void pingSentAndReceived(qint64 msec);

 private:
  void nextPing();

  void pingReceived(quint16 sequence);

 private:
  QHostAddress m_gateway;
  QHostAddress m_source;
  quint16 m_sequence = 0;

  class PingSendData {
   public:
    PingSendData() {
      timestamp = -1;
      latency = -1;
      sequence = 0;
    }
    qint64 timestamp;
    qint64 latency;
    quint16 sequence;
  };
  QVector<PingSendData> m_pingData;

  QTimer m_pingTimer;
  PingSender* m_pingSender = nullptr;

#ifdef UNIT_TEST
  friend class TestConnectionHealth;
#endif
};

#endif  // PINGHELPER_H
