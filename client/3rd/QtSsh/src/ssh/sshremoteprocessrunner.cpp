/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sshremoteprocessrunner.h"

#include "sshconnectionmanager.h"
#include "sshpseudoterminal.h"


/*!
    \class QSsh::SshRemoteProcessRunner

    \brief The SshRemoteProcessRunner class is a convenience class for
    running a remote process over an SSH connection.
*/

namespace QSsh {
namespace Internal {
namespace {
enum State { Inactive, Connecting, Connected, ProcessRunning };
} // anonymous namespace

class SshRemoteProcessRunnerPrivate
{
public:
    SshRemoteProcessRunnerPrivate() : m_state(Inactive) {}

    SshRemoteProcess::Ptr m_process;
    SshConnection *m_connection;
    bool m_runInTerminal;
    SshPseudoTerminal m_terminal;
    QByteArray m_command;
    QSsh::SshError m_lastConnectionError;
    QString m_lastConnectionErrorString;
    SshRemoteProcess::ExitStatus m_exitStatus;
    SshRemoteProcess::Signal m_exitSignal;
    QByteArray m_stdout;
    QByteArray m_stderr;
    int m_exitCode;
    QString m_processErrorString;
    State m_state;
};

} // namespace Internal

using namespace Internal;

SshRemoteProcessRunner::SshRemoteProcessRunner(QObject *parent)
    : QObject(parent), d(new SshRemoteProcessRunnerPrivate)
{
}

SshRemoteProcessRunner::~SshRemoteProcessRunner()
{
    disconnect();
    setState(Inactive);
    delete d;
}

void SshRemoteProcessRunner::run(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    QSSH_ASSERT_AND_RETURN(d->m_state == Inactive);

    d->m_runInTerminal = false;
    runInternal(command, sshParams);
}

void SshRemoteProcessRunner::runInTerminal(const QByteArray &command,
    const SshPseudoTerminal &terminal, const SshConnectionParameters &sshParams)
{
    d->m_terminal = terminal;
    d->m_runInTerminal = true;
    runInternal(command, sshParams);
}

void SshRemoteProcessRunner::runInternal(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    setState(Connecting);

    d->m_lastConnectionError = SshNoError;
    d->m_lastConnectionErrorString.clear();
    d->m_processErrorString.clear();
    d->m_exitSignal = SshRemoteProcess::NoSignal;
    d->m_exitCode = -1;
    d->m_command = command;
    d->m_connection = QSsh::acquireConnection(sshParams);
    connect(d->m_connection, &SshConnection::error,
            this, &SshRemoteProcessRunner::handleConnectionError);
    connect(d->m_connection, &SshConnection::disconnected,
            this, &SshRemoteProcessRunner::handleDisconnected);
    if (d->m_connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->m_connection, &SshConnection::connected, this, &SshRemoteProcessRunner::handleConnected);
        if (d->m_connection->state() == SshConnection::Unconnected)
            d->m_connection->connectToHost();
    }
}

void SshRemoteProcessRunner::handleConnected()
{
    QSSH_ASSERT_AND_RETURN(d->m_state == Connecting);
    setState(Connected);

    d->m_process = d->m_connection->createRemoteProcess(d->m_command);
    connect(d->m_process.data(), &SshRemoteProcess::started,
            this, &SshRemoteProcessRunner::handleProcessStarted);
    connect(d->m_process.data(), &SshRemoteProcess::closed,
            this, &SshRemoteProcessRunner::handleProcessFinished);
    connect(d->m_process.data(), &SshRemoteProcess::readyReadStandardOutput,
            this, &SshRemoteProcessRunner::handleStdout);
    connect(d->m_process.data(), &SshRemoteProcess::readyReadStandardError,
            this, &SshRemoteProcessRunner::handleStderr);
    if (d->m_runInTerminal)
        d->m_process->requestTerminal(d->m_terminal);
    d->m_process->start();
}

void SshRemoteProcessRunner::handleConnectionError(QSsh::SshError error)
{
    d->m_lastConnectionError = error;
    d->m_lastConnectionErrorString = d->m_connection->errorString();
    handleDisconnected();
    emit connectionError();
}

void SshRemoteProcessRunner::handleDisconnected()
{
    QSSH_ASSERT_AND_RETURN(d->m_state == Connecting || d->m_state == Connected
        || d->m_state == ProcessRunning);
    setState(Inactive);
}

void SshRemoteProcessRunner::handleProcessStarted()
{
    QSSH_ASSERT_AND_RETURN(d->m_state == Connected);

    setState(ProcessRunning);
    emit processStarted();
}

void SshRemoteProcessRunner::handleProcessFinished(int exitStatus)
{
    d->m_exitStatus = static_cast<SshRemoteProcess::ExitStatus>(exitStatus);
    switch (d->m_exitStatus) {
    case SshRemoteProcess::FailedToStart:
        QSSH_ASSERT_AND_RETURN(d->m_state == Connected);
        break;
    case SshRemoteProcess::CrashExit:
        QSSH_ASSERT_AND_RETURN(d->m_state == ProcessRunning);
        d->m_exitSignal = d->m_process->exitSignal();
        break;
    case SshRemoteProcess::NormalExit:
        QSSH_ASSERT_AND_RETURN(d->m_state == ProcessRunning);
        d->m_exitCode = d->m_process->exitCode();
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Impossible exit status.");
    }
    d->m_processErrorString = d->m_process->errorString();
    setState(Inactive);
    emit processClosed(exitStatus);
}

void SshRemoteProcessRunner::handleStdout()
{
    d->m_stdout += d->m_process->readAllStandardOutput();
    emit readyReadStandardOutput();
}

void SshRemoteProcessRunner::handleStderr()
{
    d->m_stderr += d->m_process->readAllStandardError();
    emit readyReadStandardError();
}

void SshRemoteProcessRunner::setState(int newState)
{
    if (d->m_state == newState)
        return;

    d->m_state = static_cast<State>(newState);
    if (d->m_state == Inactive) {
        if (d->m_process) {
            disconnect(d->m_process.data(), 0, this, 0);
            d->m_process->close();
            d->m_process.clear();
        }
        if (d->m_connection) {
            disconnect(d->m_connection, 0, this, 0);
            QSsh::releaseConnection(d->m_connection);
            d->m_connection = 0;
        }
    }
}

QByteArray SshRemoteProcessRunner::command() const { return d->m_command; }
SshError SshRemoteProcessRunner::lastConnectionError() const { return d->m_lastConnectionError; }
QString SshRemoteProcessRunner::lastConnectionErrorString() const {
    return d->m_lastConnectionErrorString;
}

bool SshRemoteProcessRunner::isProcessRunning() const
{
    return d->m_process && d->m_process->isRunning();
}

SshRemoteProcess::ExitStatus SshRemoteProcessRunner::processExitStatus() const
{
    QSSH_ASSERT(!isProcessRunning());
    return d->m_exitStatus;
}

SshRemoteProcess::Signal SshRemoteProcessRunner::processExitSignal() const
{
    QSSH_ASSERT(processExitStatus() == SshRemoteProcess::CrashExit);
    return d->m_exitSignal;
}

int SshRemoteProcessRunner::processExitCode() const
{
    QSSH_ASSERT(processExitStatus() == SshRemoteProcess::NormalExit);
    return d->m_exitCode;
}

QString SshRemoteProcessRunner::processErrorString() const
{
    return d->m_processErrorString;
}

QByteArray SshRemoteProcessRunner::readAllStandardOutput()
{
    const QByteArray data = d->m_stdout;
    d->m_stdout.clear();
    return data;
}

QByteArray SshRemoteProcessRunner::readAllStandardError()
{
    const QByteArray data = d->m_stderr;
    d->m_stderr.clear();
    return data;
}

void SshRemoteProcessRunner::writeDataToProcess(const QByteArray &data)
{
    QSSH_ASSERT(isProcessRunning());
    d->m_process->write(data);
}

void SshRemoteProcessRunner::sendSignalToProcess(SshRemoteProcess::Signal signal)
{
    QSSH_ASSERT(isProcessRunning());
    d->m_process->sendSignal(signal);
}

void SshRemoteProcessRunner::cancel()
{
    setState(Inactive);
}

} // namespace QSsh
