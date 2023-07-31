/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "pinghelper.h"

#include <QDateTime>
#include <cmath>

#include "dnspingsender.h"
#include "leakdetector.h"
#include "logger.h"
#include "pingsender.h"
#include "pingsenderfactory.h"

// Any X seconds, a new ping.
constexpr uint32_t PING_TIMEOUT_SEC = 1;

// Maximum window size for ping statistics.
constexpr int PING_STATS_WINDOW = 32;

namespace {
Logger logger("PingHelper");
}

PingHelper::PingHelper() {
  MZ_COUNT_CTOR(PingHelper);

  m_sequence = 0;
  m_pingData.resize(PING_STATS_WINDOW);

  connect(&m_pingTimer, &QTimer::timeout, this, &PingHelper::nextPing);
}

PingHelper::~PingHelper() { MZ_COUNT_DTOR(PingHelper); }

void PingHelper::start(const QString& serverIpv4Gateway,
                       const QString& deviceIpv4Address) {
  logger.debug() << "PingHelper activated for server:"
                 << logger.sensitive(serverIpv4Gateway);

  m_gateway = QHostAddress(serverIpv4Gateway);
  m_source = QHostAddress(deviceIpv4Address.section('/', 0, 0));
  m_pingSender = PingSenderFactory::create(m_source, this);

  // Some platforms require root access to send and receive ICMP pings. If
  // we happen to be on one of these unlucky devices, create a DnsPingSender
  // instead.
  if (!m_pingSender->isValid()) {
    delete m_pingSender;
    m_pingSender = new DnsPingSender(m_source, this);
  }

  connect(m_pingSender, &PingSender::recvPing, this, &PingHelper::pingReceived,
          Qt::QueuedConnection);
  connect(m_pingSender, &PingSender::criticalPingError, this,
          []() { logger.info() << "Encountered Unrecoverable ping error"; });

  // Reset the ping statistics
  m_sequence = 0;
  for (int i = 0; i < PING_STATS_WINDOW; i++) {
    m_pingData[i].timestamp = -1;
    m_pingData[i].latency = -1;
    m_pingData[i].sequence = 0;
  }

  m_pingTimer.start(PING_TIMEOUT_SEC * 1000);
}

void PingHelper::stop() {
  logger.debug() << "PingHelper deactivated";

  if (m_pingSender) {
    delete m_pingSender;
    m_pingSender = nullptr;
  }

  m_pingTimer.stop();
}

void PingHelper::nextPing() {
#ifdef MZ_DEBUG
  logger.debug() << "Sending ping seq:" << m_sequence;
#endif

  // The ICMP sequence number is used to match replies with their originating
  // request, and serves as an index into the circular buffer. Overflows of
  // the sequence number acceptable.
  int index = m_sequence % PING_STATS_WINDOW;
  m_pingData[index].timestamp = QDateTime::currentMSecsSinceEpoch();
  m_pingData[index].latency = -1;
  m_pingData[index].sequence = m_sequence;
  m_pingSender->sendPing(m_gateway, m_sequence);

  m_sequence++;
}

void PingHelper::pingReceived(quint16 sequence) {
  int index = sequence % PING_STATS_WINDOW;
  if (m_pingData[index].sequence == sequence) {
    qint64 sendTime = m_pingData[index].timestamp;
    m_pingData[index].latency = QDateTime::currentMSecsSinceEpoch() - sendTime;
    emit pingSentAndReceived(m_pingData[index].latency);
#ifdef MZ_DEBUG
    logger.debug() << "Ping answer received seq:" << sequence
                   << "avg:" << latency()
                   << "loss:" << QString("%1%").arg(loss() * 100.0)
                   << "stddev:" << stddev();
#endif
  }
}

uint PingHelper::latency() const {
  int recvCount = 0;
  qint64 totalMsec = 0;

  for (const PingSendData& data : m_pingData) {
    if (data.latency < 0) {
      continue;
    }
    recvCount++;
    totalMsec += data.latency;
  }

  if (recvCount <= 0) {
    return 0.0;
  }

  // Add half the denominator to produce nearest-integer rounding.
  totalMsec += recvCount / 2;
  return static_cast<uint>(totalMsec / recvCount);
}

uint PingHelper::stddev() const {
  int recvCount = 0;
  qint64 totalVariance = 0;
  uint average = PingHelper::latency();

  for (const PingSendData& data : m_pingData) {
    if (data.latency < 0) {
      continue;
    }
    recvCount++;
    totalVariance += (average - data.latency) * (average - data.latency);
  }

  if (recvCount <= 0) {
    return 0.0;
  }

  return std::sqrt((double)totalVariance / recvCount);
}

uint PingHelper::maximum() const {
  uint maxRtt = 0;

  for (const PingSendData& data : m_pingData) {
    if (data.latency < 0) {
      continue;
    }
    if (data.latency > maxRtt &&
        data.latency < std::numeric_limits<uint>::max()) {
      maxRtt = static_cast<uint>(data.latency);
    }
  }
  return maxRtt;
}

double PingHelper::loss() const {
  int sendCount = 0;
  int recvCount = 0;
  // Don't count pings that are possibly still in flight as losses.
  qint64 sendBefore =
      QDateTime::currentMSecsSinceEpoch() - (PING_TIMEOUT_SEC * 1000);

  for (const PingSendData& data : m_pingData) {
    if (data.latency >= 0) {
      recvCount++;
      sendCount++;
    } else if ((data.timestamp > 0) && (data.timestamp < sendBefore)) {
      sendCount++;
    }
  }

  if (sendCount <= 0) {
    return 0.0;
  }
  return (double)(sendCount - recvCount) / PING_STATS_WINDOW;
}
