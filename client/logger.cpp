#include "logger.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QMetaEnum>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUrl>

#include <iostream>

#include "version.h"
#include "utilities.h"

#ifdef AMNEZIA_DESKTOP
#include <core/ipcclient.h>
#endif

QFile Logger::m_file;
QTextStream Logger::m_textStream;
QString Logger::m_logFileName = QString("%1.log").arg(APPLICATION_NAME);

void debugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (msg.simplified().isEmpty()) {
        return;
    }

    // Skip annoying messages from Qt
    if (msg.startsWith("Unknown property") || msg.startsWith("Could not create pixmap") || msg.startsWith("Populating font")) {
        return;
    }

    Logger::m_textStream << qFormatLogMessage(type, context, msg) << Qt::endl << Qt::flush;
    Logger::appendAllLog(qFormatLogMessage(type, context, msg));

    std::cout << qFormatLogMessage(type, context, msg).toStdString() << std::endl << std::flush;
}

Logger &Logger::Instance()
{
    static Logger s;
    return s;
}

void Logger::appendSshLog(const QString &log)
{
    QString dt = QDateTime::currentDateTime().toString();
    Instance().m_sshLog.append(dt + ": " + log + "\n");
    emit Instance().sshLogChanged(Instance().sshLog());
}

void Logger::appendAllLog(const QString &log)
{
    Instance().m_allLog.append(log + "\n");
    emit Instance().allLogChanged(Instance().allLog());
}

bool Logger::init()
{
    qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} %{type} %{message}");

    QString path = userLogsDir();
    QDir appDir(path);
    if (!appDir.mkpath(path)) {
        return false;
    }

    m_file.setFileName(appDir.filePath(m_logFileName));
    if (!m_file.open(QIODevice::Append)) {
        qWarning() << "Cannot open log file:" << m_logFileName;
        return false;
    }
    m_file.setTextModeEnabled(true);
    m_textStream.setDevice(&m_file);

#ifndef QT_DEBUG
    qInstallMessageHandler(debugMessageHandler);
#endif

    return true;
}

void Logger::deInit()
{
    qInstallMessageHandler(nullptr);
    qSetMessagePattern("%{message}");
    m_textStream.setDevice(nullptr);
    m_file.close();
}

QString Logger::userLogsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/log";
}

QString Logger::userLogsFilePath()
{
    return userLogsDir() + QDir::separator() + m_logFileName;
}

QString Logger::getLogFile()
{
    m_file.flush();
    QFile file(userLogsFilePath());

    file.open(QIODevice::ReadOnly);
    return file.readAll();
}

bool Logger::openLogsFolder()
{
    QString path = userLogsDir();
#ifdef Q_OS_WIN
    path = "file:///" + path;
#endif
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        qWarning() << "Can't open url:" << path;
        return false;
    }
    return true;
}

bool Logger::openServiceLogsFolder()
{
    QString path = Utils::systemLogPath();
    path = "file:///" + path;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    return true;
}

QString Logger::appLogFileNamePath()
{
    return m_file.fileName();
}

void Logger::clearLogs()
{
    bool isLogActive = m_file.isOpen();
    m_file.close();

    QFile file(userLogsFilePath());

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.resize(0);
    file.close();

    if (isLogActive) {
        init();
    }
}

void Logger::clearServiceLogs()
{
#ifdef AMNEZIA_DESKTOP
    IpcClient *m_IpcClient = new IpcClient;

    if (!m_IpcClient->isSocketConnected()) {
        if (!IpcClient::init(m_IpcClient)) {
            qWarning() << "Error occurred when init IPC client";
            return;
        }
    }

    if (m_IpcClient->Interface()) {
        m_IpcClient->Interface()->setLogsEnabled(false);
        m_IpcClient->Interface()->cleanUp();
    }
    else {
        qWarning() << "Error occurred cleaning up service logs";
    }
#endif
}

void Logger::cleanUp()
{
    clearLogs();
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.removeRecursively();

    clearServiceLogs();
}

Logger::Log::Log(Logger* logger, LogLevel logLevel)
    : m_logger(logger), m_logLevel(logLevel), m_data(new Data()) {}

Logger::Log::~Log() {
    qDebug() << "Mozilla" << m_logger->className() << m_data->m_buffer.trimmed();
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
