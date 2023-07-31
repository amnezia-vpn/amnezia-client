#include "logger.h"

#include <QDir>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QStandardPaths>

#include <iostream>

#include "version.h"
#include "utilities.h"

QFile Logger::m_file;
QTextStream Logger::m_textStream;
QString Logger::m_logFileName = QString("%1.log").arg(SERVICE_NAME);

void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (msg.simplified().isEmpty()) {
        return;
    }

    Logger::m_textStream << qFormatLogMessage(type, context, msg) << Qt::endl << Qt::flush;

    std::cout << qFormatLogMessage(type, context, msg).toStdString() << std::endl << std::flush;
}

bool Logger::init()
{
    if (m_file.isOpen()) return true;

    QString path = Utils::systemLogPath();
    QDir appDir(path);
    if (!appDir.mkpath(path)) {
        return false;
    }

    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} %{type} %{message}");

    m_file.setFileName(appDir.filePath(m_logFileName));
    if (!m_file.open(QIODevice::Append)) {
        qWarning() << "Cannot open log file:" << m_logFileName;
        return false;
    }
    m_file.setTextModeEnabled(true);
    m_textStream.setDevice(&m_file);
    qInstallMessageHandler(debugMessageHandler);

    return true;
}

void Logger::deinit()
{
    m_file.close();
    m_textStream.setDevice(nullptr);
    qInstallMessageHandler(nullptr);
}

QString Logger::serviceLogFileNamePath()
{
    return m_file.fileName();
}

void Logger::clearLogs()
{
    bool isLogActive = m_file.isOpen();
    m_file.close();


    QString path = Utils::systemLogPath();
    QDir appDir(path);
    QFile file;
    file.setFileName(appDir.filePath(m_logFileName));

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.resize(0);
    file.close();

    if (isLogActive) {
        init();
    }
}

void Logger::cleanUp()
{
    clearLogs();
    deinit();

    QString path = Utils::systemLogPath();
    QDir appDir(path);

    {
        QFile file;
        file.setFileName(appDir.filePath(m_logFileName));
        file.remove();
    }
    {
        QFile file;
        file.setFileName(appDir.filePath("openvpn.log"));
        file.remove();
    }

#ifdef Q_OS_WINDOWS
    QDir dir(Utils::systemLogPath());
    dir.removeRecursively();
#endif
}


Logger::Log::Log(Logger* logger, LogLevel logLevel)
    : m_logger(logger), m_logLevel(logLevel), m_data(new Data()) {}

Logger::Log::~Log() {
    qDebug() << "Amnezia" << m_logger->className() << m_data->m_buffer.trimmed();
    delete m_data;
}

Logger::Log Logger::error() { return Log(this, LogLevel::Error); }
Logger::Log Logger::warning() { return Log(this, LogLevel::Warning); }
Logger::Log Logger::info() { return Log(this, LogLevel::Info); }
Logger::Log Logger::debug() { return Log(this, LogLevel::Debug); }
QString Logger::sensitive(const QString& input) {
#ifdef Q_DEBUG
    return input;
#else
    Q_UNUSED(input);
    return QString(8, 'X');
#endif
}


#define CREATE_LOG_OP_REF(x)                  \
Logger::Log& Logger::Log::operator<<(x t) {   \
        m_data->m_ts << t << ' ';                 \
        return *this;                             \
}

CREATE_LOG_OP_REF(uint64_t);
CREATE_LOG_OP_REF(const char*);
CREATE_LOG_OP_REF(const QString&);
CREATE_LOG_OP_REF(const QByteArray&);
CREATE_LOG_OP_REF(const void*);

#undef CREATE_LOG_OP_REF

Logger::Log& Logger::Log::operator<<(const QStringList& t) {
    m_data->m_ts << '[' << t.join(",") << ']' << ' ';
    return *this;
}

Logger::Log& Logger::Log::operator<<(const QJsonObject& t) {
    m_data->m_ts << QJsonDocument(t).toJson(QJsonDocument::Indented) << ' ';
    return *this;
}

Logger::Log& Logger::Log::operator<<(QTextStreamFunction t) {
    m_data->m_ts << t;
    return *this;
}

void Logger::Log::addMetaEnum(quint64 value, const QMetaObject* meta,
                              const char* name) {
    QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));

    QString out;
    QTextStream ts(&out);

    if (const char* scope = me.scope()) {
        ts << scope << "::";
    }

    const char* key = me.valueToKey(static_cast<int>(value));
    const bool scoped = me.isScoped();
    if (scoped || !key) {
        ts << me.enumName() << (!key ? "(" : "::");
    }

    if (key) {
        ts << key;
    } else {
        ts << value << ")";
    }

    m_data->m_ts << out;
}
