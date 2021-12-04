/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "loghandler.h"
#include "constants.h"
#include "logger.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageLogContext>
#include <QMutexLocker>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

#ifdef MVPN_ANDROID
#  include <android/log.h>
#endif

constexpr qint64 LOG_MAX_FILE_SIZE = 204800;
constexpr const char* LOG_FILENAME = "mozillavpn.txt";

namespace {
QMutex s_mutex;
QString s_location =
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
LogHandler* s_instance = nullptr;

LogLevel qtTypeToLogLevel(QtMsgType type) {
  switch (type) {
    case QtDebugMsg:
      return Debug;
    case QtInfoMsg:
      return Info;
    case QtWarningMsg:
      return Warning;
    case QtCriticalMsg:
      [[fallthrough]];
    case QtFatalMsg:
      return Error;
    default:
      return Debug;
  }
}

}  // namespace

// static
LogHandler* LogHandler::instance() {
  QMutexLocker lock(&s_mutex);
  return maybeCreate(lock);
}

// static
void LogHandler::messageQTHandler(QtMsgType type,
                                  const QMessageLogContext& context,
                                  const QString& message) {
  QMutexLocker lock(&s_mutex);
  maybeCreate(lock)->addLog(Log(qtTypeToLogLevel(type), context.file,
                                context.function, context.line, message),
                            lock);
}

// static
void LogHandler::messageHandler(LogLevel logLevel, const QStringList& modules,
                                const QString& className,
                                const QString& message) {
  QMutexLocker lock(&s_mutex);
  maybeCreate(lock)->addLog(Log(logLevel, modules, className, message), lock);
}

// static
LogHandler* LogHandler::maybeCreate(const QMutexLocker& proofOfLock) {
  if (!s_instance) {
    LogLevel minLogLevel = Debug;  // TODO: in prod, we should log >= warning
    QStringList modules;
    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
    if (pe.contains("MOZVPN_LEVEL")) {
      QString level = pe.value("MOZVPN_LEVEL");
      if (level == "info")
        minLogLevel = Info;
      else if (level == "warning")
        minLogLevel = Warning;
      else if (level == "error")
        minLogLevel = Error;
    }

    if (pe.contains("MOZVPN_LOG")) {
      QStringList parts = pe.value("MOZVPN_LOG").split(",");
      for (const QString& part : parts) {
        modules.append(part.trimmed());
      }
    }

    s_instance = new LogHandler(minLogLevel, modules, proofOfLock);
  }

  return s_instance;
}

// static
void LogHandler::prettyOutput(QTextStream& out, const LogHandler::Log& log) {
  out << "[" << log.m_dateTime.toString("dd.MM.yyyy hh:mm:ss.zzz") << "] ";

  switch (log.m_logLevel) {
    case Debug:
      out << "Debug: ";
      break;
    case Info:
      out << "Info: ";
      break;
    case Warning:
      out << "Warning: ";
      break;
    case Error:
      out << "Error: ";
      break;
    default:
      out << "?!?: ";
      break;
  }

  if (log.m_fromQT) {
    out << log.m_message;

    if (!log.m_file.isEmpty() || !log.m_function.isEmpty()) {
      out << " (";

      if (!log.m_file.isEmpty()) {
        int pos = log.m_file.lastIndexOf("/");
        out << log.m_file.right(log.m_file.length() - pos - 1);

        if (log.m_line >= 0) {
          out << ":" << log.m_line;
        }

        if (!log.m_function.isEmpty()) {
          out << ", ";
        }
      }

      if (!log.m_function.isEmpty()) {
        out << log.m_function;
      }

      out << ")";
    }
  } else {
    out << "(" << log.m_modules.join("|") << " - " << log.m_className << ") "
        << log.m_message;
  }

  out << Qt::endl;
}

// static
void LogHandler::enableDebug() {
  QMutexLocker lock(&s_mutex);
  maybeCreate(lock)->m_showDebug = true;
}

LogHandler::LogHandler(LogLevel minLogLevel, const QStringList& modules,
                       const QMutexLocker& proofOfLock)
    : m_minLogLevel(minLogLevel), m_modules(modules) {
  Q_UNUSED(proofOfLock);

#if defined(QT_DEBUG) || defined(MVPN_WASM)
  m_showDebug = true;
#endif

  if (!s_location.isEmpty()) {
    openLogFile(proofOfLock);
  }
}

void LogHandler::addLog(const Log& log, const QMutexLocker& proofOfLock) {
  if (!matchLogLevel(log, proofOfLock)) {
    return;
  }

  if (!matchModule(log, proofOfLock)) {
    return;
  }

  if (m_output) {
    prettyOutput(*m_output, log);
  }

  if ((log.m_logLevel != LogLevel::Debug) || m_showDebug) {
    QTextStream out(stderr);
    prettyOutput(out, log);
  }

  QByteArray buffer;
  {
    QTextStream out(&buffer);
    prettyOutput(out, log);
  }

  emit logEntryAdded(buffer);

#if defined(MVPN_ANDROID) && defined(QT_DEBUG)
  const char* str = buffer.constData();
  if (str) {
    __android_log_write(ANDROID_LOG_DEBUG, "mozillavpn", str);
  }
#endif
}

bool LogHandler::matchModule(const Log& log,
                             const QMutexLocker& proofOfLock) const {
  Q_UNUSED(proofOfLock);

  // Let's include QT logs always.
  if (log.m_fromQT) {
    return true;
  }

  // If no modules has been specified, let's include all.
  if (m_modules.isEmpty()) {
    return true;
  }

  for (const QString& module : log.m_modules) {
    if (m_modules.contains(module)) {
      return true;
    }
  }

  return false;
}

bool LogHandler::matchLogLevel(const Log& log,
                               const QMutexLocker& proofOfLock) const {
  Q_UNUSED(proofOfLock);
  return log.m_logLevel >= m_minLogLevel;
}

// static
void LogHandler::writeLogs(QTextStream& out) {
  QMutexLocker lock(&s_mutex);

  if (!s_instance || !s_instance->m_logFile) {
    return;
  }

  QString logFileName = s_instance->m_logFile->fileName();
  s_instance->closeLogFile(lock);

  {
    QFile file(logFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return;
    }

    out << file.readAll();
  }

  s_instance->openLogFile(lock);
}

// static
void LogHandler::cleanupLogs() {
  QMutexLocker lock(&s_mutex);
  cleanupLogFile(lock);
}

// static
void LogHandler::cleanupLogFile(const QMutexLocker& proofOfLock) {
  if (!s_instance || !s_instance->m_logFile) {
    return;
  }

  QString logFileName = s_instance->m_logFile->fileName();
  s_instance->closeLogFile(proofOfLock);

  {
    QFile file(logFileName);
    file.remove();
  }

  s_instance->openLogFile(proofOfLock);
}

// static
void LogHandler::setLocation(const QString& path) {
  QMutexLocker lock(&s_mutex);
  s_location = path;

  if (s_instance && s_instance->m_logFile) {
    cleanupLogFile(lock);
  }
}

void LogHandler::openLogFile(const QMutexLocker& proofOfLock) {
  Q_UNUSED(proofOfLock);
  Q_ASSERT(!m_logFile);
  Q_ASSERT(!m_output);

  QDir appDataLocation(s_location);
  if (!appDataLocation.exists()) {
    QDir tmp(s_location);
    tmp.cdUp();
    if (!tmp.exists()) {
      return;
    }
    if (!tmp.mkdir(appDataLocation.dirName())) {
      return;
    }
  }

  QString logFileName = appDataLocation.filePath(LOG_FILENAME);
  m_logFile = new QFile(logFileName);
  if (m_logFile->size() > LOG_MAX_FILE_SIZE) {
    m_logFile->remove();
  }

  if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append |
                       QIODevice::Text)) {
    delete m_logFile;
    m_logFile = nullptr;
    return;
  }

  m_output = new QTextStream(m_logFile);

  addLog(Log(Debug, QStringList{LOG_MAIN}, "LogHandler",
             QString("Log file: %1").arg(logFileName)),
         proofOfLock);
}

void LogHandler::closeLogFile(const QMutexLocker& proofOfLock) {
  Q_UNUSED(proofOfLock);

  if (m_logFile) {
    delete m_output;
    m_output = nullptr;

    delete m_logFile;
    m_logFile = nullptr;
  }
}
