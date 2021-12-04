/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "logger.h"
#include "loghandler.h"

Logger::Logger(const QString& module, const QString& className)
    : Logger(QStringList({module}), className) {}

Logger::Logger(const QStringList& modules, const QString& className)
    : m_modules(modules), m_className(className) {}

Logger::Log Logger::error() { return Log(this, LogLevel::Error); }
Logger::Log Logger::warning() { return Log(this, LogLevel::Warning); }
Logger::Log Logger::info() { return Log(this, LogLevel::Info); }
Logger::Log Logger::debug() { return Log(this, LogLevel::Debug); }

Logger::Log::Log(Logger* logger, LogLevel logLevel)
    : m_logger(logger), m_logLevel(logLevel), m_data(new Data()) {}

Logger::Log::~Log() {
  LogHandler::messageHandler(m_logLevel, m_logger->modules(),
                             m_logger->className(), m_data->m_buffer.trimmed());
  delete m_data;
}

#define CREATE_LOG_OP_REF(x)                  \
  Logger::Log& Logger::Log::operator<<(x t) { \
    m_data->m_ts << t << ' ';                 \
    return *this;                             \
  }

CREATE_LOG_OP_REF(uint64_t);
CREATE_LOG_OP_REF(const char*);
CREATE_LOG_OP_REF(const QString&);
CREATE_LOG_OP_REF(const QByteArray&);
CREATE_LOG_OP_REF(void*);

#undef CREATE_LOG_OP_REF

Logger::Log& Logger::Log::operator<<(const QStringList& t) {
  m_data->m_ts << '[' << t.join(",") << ']' << ' ';
  return *this;
}

Logger::Log& Logger::Log::operator<<(QTextStreamFunction t) {
  m_data->m_ts << t;
  return *this;
}

// static
QString Logger::sensitive(const QString& input) {
#ifdef QT_DEBUG
  return input;
#else
  return QString(input.length(), 'X');
#endif
}
