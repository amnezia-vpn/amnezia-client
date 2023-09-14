/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowstunnellogger.h"

#include <QDateTime>

#include "leakdetector.h"
#include "logger.h"

/* The ring logger format used by the Wireguard DLL is as follows, assuming
 * no padding:
 *
 * struct {
 *   uint32_t magic;
 *   uint32_t index;
 *   struct {
 *     uint64_t timestamp;
 *     char message[512];
 *   } ring[2048];
 * };
 */

constexpr uint32_t RINGLOG_POLL_MSEC = 250;
constexpr uint32_t RINGLOG_MAGIC_HEADER = 0xbadbabe;
constexpr uint32_t RINGLOG_INDEX_OFFSET = 4;
constexpr uint32_t RINGLOG_HEADER_SIZE = 8;
constexpr uint32_t RINGLOG_MAX_ENTRIES = 2048;
constexpr uint32_t RINGLOG_MESSAGE_SIZE = 512;
constexpr uint32_t RINGLOG_TIMESTAMP_SIZE = 8;
constexpr uint32_t RINGLOG_FILE_SIZE =
    RINGLOG_HEADER_SIZE +
    ((RINGLOG_MESSAGE_SIZE + RINGLOG_TIMESTAMP_SIZE) * RINGLOG_MAX_ENTRIES);

namespace {
Logger logger("tunnel.dll");
}  // namespace

WindowsTunnelLogger::WindowsTunnelLogger(const QString& filename,
                                         QObject* parent)
    : QObject(parent), m_logfile(filename, this), m_timer(this) {
  MZ_COUNT_CTOR(WindowsTunnelLogger);

  m_startTime = QDateTime::currentMSecsSinceEpoch() * 1000000;
  m_logindex = -1;

  connect(&m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
  m_timer.start(RINGLOG_POLL_MSEC);
}

WindowsTunnelLogger::~WindowsTunnelLogger() {
  MZ_COUNT_DTOR(WindowsTunnelLogger);
  if (m_logdata) {
    timeout();

    m_logfile.unmap(m_logdata);
    m_logdata = nullptr;
  }
}

bool WindowsTunnelLogger::openLogData() {
  if (m_logdata) {
    return true;
  }
  if (!m_logfile.open(QIODevice::ReadOnly)) {
    return false;
  }

  m_logdata = m_logfile.map(0, RINGLOG_FILE_SIZE);
  if (!m_logdata) {
    m_logfile.close();
    return false;
  }

  // Check for a valid magic header
  uint32_t magic;
  memcpy(&magic, m_logdata, 4);
  logger.debug() << "Opening tunnel log file" << m_logfile.fileName();
  if (magic != RINGLOG_MAGIC_HEADER) {
    logger.error() << "Unexpected magic header:" << QString::number(magic, 16);
    m_logfile.unmap(m_logdata);
    m_logfile.close();
    m_logdata = nullptr;
    return false;
  }

  return true;
}

int WindowsTunnelLogger::nextIndex() {
  qint32 value;
  memcpy(&value, m_logdata + RINGLOG_INDEX_OFFSET, 4);
  return value % RINGLOG_MAX_ENTRIES;
}

void WindowsTunnelLogger::process(int index) {
  Q_ASSERT(index >= 0);
  Q_ASSERT(index < RINGLOG_MAX_ENTRIES);
  size_t offset = static_cast<size_t>(index) *
                  (RINGLOG_TIMESTAMP_SIZE + RINGLOG_MESSAGE_SIZE);
  uchar* data = m_logdata + RINGLOG_HEADER_SIZE + offset;

  quint64 timestamp;
  memcpy(&timestamp, data, RINGLOG_TIMESTAMP_SIZE);
  if (timestamp <= m_startTime) {
    return;
  }
  QByteArray message((const char*)data + RINGLOG_TIMESTAMP_SIZE,
                     RINGLOG_MESSAGE_SIZE);
  int nullIndex = message.indexOf(0x0);
  if (nullIndex >= 0) {
    message.truncate(nullIndex);
  }
  logger.info() << QString::fromUtf8(message);
}

void WindowsTunnelLogger::timeout() {
  if (!openLogData()) {
    return;
  }

  /* On the first pass, scan all log messages. */
  if (m_logindex < 0) {
    m_logindex = nextIndex();
    process(m_logindex);
    m_logindex = (m_logindex + 1) % RINGLOG_MAX_ENTRIES;
  }

  /* Report new messages. */
  while (m_logindex != nextIndex()) {
    process(m_logindex);
    m_logindex = (m_logindex + 1) % RINGLOG_MAX_ENTRIES;
  }
}
