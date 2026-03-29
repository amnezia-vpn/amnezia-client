#include "ipcserverprocess.h"
#include "ipc.h"
#include <QProcess>
#include <QDir>

namespace {
QString processErrorToString(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart: return "FailedToStart";
    case QProcess::Crashed: return "Crashed";
    case QProcess::Timedout: return "Timedout";
    case QProcess::ReadError: return "ReadError";
    case QProcess::WriteError: return "WriteError";
    case QProcess::UnknownError: return "UnknownError";
    }
    return "UnknownProcessError";
}

QString processStateToString(QProcess::ProcessState state)
{
    switch (state) {
    case QProcess::NotRunning: return "NotRunning";
    case QProcess::Starting: return "Starting";
    case QProcess::Running: return "Running";
    }
    return "UnknownProcessState";
}

QString exitStatusToString(QProcess::ExitStatus status)
{
    switch (status) {
    case QProcess::NormalExit: return "NormalExit";
    case QProcess::CrashExit: return "CrashExit";
    }
    return "UnknownExitStatus";
}
}

#ifndef Q_OS_IOS

IpcServerProcess::IpcServerProcess(QObject *parent) :
    IpcProcessInterfaceSource(parent),
    m_process(QSharedPointer<QProcess>(new QProcess()))
{
    connect(m_process.data(), &QProcess::errorOccurred, this, &IpcServerProcess::errorOccurred);
    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &IpcServerProcess::finished);
    connect(m_process.data(), &QProcess::readyRead, this, &IpcServerProcess::readyRead);
    connect(m_process.data(), &QProcess::readyReadStandardError, this, &IpcServerProcess::readyReadStandardError);
    connect(m_process.data(), &QProcess::readyReadStandardOutput, this, &IpcServerProcess::readyReadStandardOutput);
    connect(m_process.data(), &QProcess::started, this, &IpcServerProcess::started);
    connect(m_process.data(), &QProcess::stateChanged, this, &IpcServerProcess::stateChanged);

    connect(m_process.data(), &QProcess::errorOccurred, [&](QProcess::ProcessError error){
        qWarning() << "IpcServerProcess errorOccurred"
                   << processErrorToString(error)
                   << "program=" << m_process->program()
                   << "arguments=" << m_process->arguments()
                   << "pid=" << m_process->processId()
                   << "state=" << processStateToString(m_process->state())
                   << "errorString=" << m_process->errorString();
    });
    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [&](int exitCode, QProcess::ExitStatus exitStatus) {
        qWarning() << "IpcServerProcess finished"
                   << "program=" << m_process->program()
                   << "arguments=" << m_process->arguments()
                   << "pid=" << m_process->processId()
                   << "exitCode=" << exitCode
                   << "exitStatus=" << exitStatusToString(exitStatus);
    });
    connect(m_process.data(), &QProcess::started, this, [&]() {
        qDebug() << "IpcServerProcess started signal"
                 << "program=" << m_process->program()
                 << "arguments=" << m_process->arguments()
                 << "pid=" << m_process->processId()
                 << "workingDirectory=" << m_process->workingDirectory();
    });
    connect(m_process.data(), &QProcess::stateChanged, this, [&](QProcess::ProcessState state) {
        qDebug() << "IpcServerProcess stateChanged"
                 << processStateToString(state)
                 << "program=" << m_process->program()
                 << "pid=" << m_process->processId();
    });

}

IpcServerProcess::~IpcServerProcess()
{
    qDebug() << "IpcServerProcess::~IpcServerProcess";
}

void IpcServerProcess::start()
{
    if (m_process->program().isEmpty()) {
        qDebug() << "IpcServerProcess failed to start, program is empty";
    }

    Utils::killProcessByName(m_process->program());
    m_process->start();
    qDebug() << "IpcServerProcess start requested"
             << "program=" << m_process->program()
             << "arguments=" << m_process->arguments()
             << "workingDirectory=" << (m_process->workingDirectory().isEmpty() ? QDir::currentPath() : m_process->workingDirectory());

    if (!m_process->waitForStarted()) {
        qWarning() << "IpcServerProcess waitForStarted failed"
                   << "program=" << m_process->program()
                   << "arguments=" << m_process->arguments()
                   << "errorString=" << m_process->errorString();
    }
}

void IpcServerProcess::terminate() {
    m_process->terminate();
}

void IpcServerProcess::kill() {
    m_process->kill();
}

void IpcServerProcess::close()
{
    m_process->close();
}

void IpcServerProcess::setArguments(const QStringList &arguments)
{
    m_process->setArguments(fblink::sanitizeArguments(m_program, arguments));
}

void IpcServerProcess::setInputChannelMode(QProcess::InputChannelMode mode)
{
     m_process->setInputChannelMode(mode);
}

void IpcServerProcess::setNativeArguments(const QString &arguments)
{
#ifdef Q_OS_WIN
    m_process->setNativeArguments(arguments);
#endif
}

void IpcServerProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    m_process->setProcessChannelMode(mode);
}

void IpcServerProcess::setProgram(int programId)
{
    m_program = static_cast<fblink::PermittedProcess>(programId);
    m_process->setProgram(fblink::permittedProcessPath(m_program));
    m_process->setArguments({});
}

void IpcServerProcess::setWorkingDirectory(const QString &dir)
{
    m_process->setWorkingDirectory(dir);
}

QByteArray IpcServerProcess::readAll()
{
    return m_process->readAll();
}

QByteArray IpcServerProcess::readAllStandardError()
{
    return m_process->readAllStandardError();
}

QByteArray IpcServerProcess::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

bool IpcServerProcess::waitForStarted() {
    return m_process->waitForStarted();
}

bool IpcServerProcess::waitForStarted(int msecs) {
    return m_process->waitForStarted(msecs);
}

bool IpcServerProcess::waitForFinished() {
    return m_process->waitForFinished();
}

bool IpcServerProcess::waitForFinished(int msecs) {
    return m_process->waitForFinished(msecs);
}

#endif
