/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef SSHREMOTECOMMAND_H
#define SSHREMOTECOMMAND_H

#include "ssh_global.h"

#include <QProcess>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace QSsh {
class SshPseudoTerminal;
namespace Internal {
class SshChannelManager;
class SshRemoteProcessPrivate;
class SshSendFacility;
} // namespace Internal

/*!
    \class QSsh::SshRemoteProcess

    \brief This class implements an SSH channel for running a remote process.

    Objects are created via SshConnection::createRemoteProcess.
    The process is started via the start() member function.
    If the process needs a pseudo terminal, you can request one
    via requestTerminal() before calling start().
    Note that this class does not support QIODevice's waitFor*() functions, i.e. it has
    no synchronous mode.
 */

// TODO: ProcessChannel
class QSSH_EXPORT SshRemoteProcess : public QIODevice
{
    Q_OBJECT

    friend class Internal::SshChannelManager;
    friend class Internal::SshRemoteProcessPrivate;

public:
    typedef QSharedPointer<SshRemoteProcess> Ptr;
    enum ExitStatus { FailedToStart, CrashExit, NormalExit };
    enum Signal {
        AbrtSignal, AlrmSignal, FpeSignal, HupSignal, IllSignal, IntSignal, KillSignal, PipeSignal,
        QuitSignal, SegvSignal, TermSignal, Usr1Signal, Usr2Signal, NoSignal
    };

    ~SshRemoteProcess();

    // QIODevice stuff
    bool atEnd() const;
    qint64 bytesAvailable() const;
    bool canReadLine() const;
    void close();
    bool isSequential() const { return true; }

    QProcess::ProcessChannel readChannel() const;
    void setReadChannel(QProcess::ProcessChannel channel);

    /*
     * Note that this is of limited value in practice, because servers are
     * usually configured to ignore such requests for security reasons.
     */
    void addToEnvironment(const QByteArray &var, const QByteArray &value);
    void clearEnvironment();

    void requestTerminal(const SshPseudoTerminal &terminal);
    void requestX11Forwarding(const QString &displayName);
    void start();

    bool isRunning() const;
    int exitCode() const;
    Signal exitSignal() const;

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    // Note: This is ignored by the OpenSSH server.
    void sendSignal(Signal signal);
    void kill() { sendSignal(KillSignal); }

signals:
    void started();

    void readyReadStandardOutput();
    void readyReadStandardError();

    /*
     * Parameter is of type ExitStatus, but we use int because of
     * signal/slot awkwardness (full namespace required).
     */
    void closed(int exitStatus);

private:
    SshRemoteProcess(const QByteArray &command, quint32 channelId,
        Internal::SshSendFacility &sendFacility);
    SshRemoteProcess(quint32 channelId, Internal::SshSendFacility &sendFacility);

    // QIODevice stuff
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    void init();
    QByteArray readAllFromChannel(QProcess::ProcessChannel channel);

    Internal::SshRemoteProcessPrivate *d;
};

} // namespace QSsh

#endif // SSHREMOTECOMMAND_H
