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

#ifndef SSHREMOTEPROCESS_P_H
#define SSHREMOTEPROCESS_P_H

#include "sshpseudoterminal.h"

#include "sshchannel_p.h"

#include <QList>
#include <QPair>
#include <QProcess>

namespace QSsh {
class SshRemoteProcess;

namespace Internal {
class SshSendFacility;
class X11DisplayInfo;

class SshRemoteProcessPrivate : public AbstractSshChannel
{
    Q_OBJECT
    friend class QSsh::SshRemoteProcess;
public:
    enum ProcessState {
        NotYetStarted, ExecRequested, StartFailed, Running, Exited
    };

    void failToStart(const QString &reason);
    void startProcess(const X11DisplayInfo &displayInfo);

signals:
    void started();
    void readyRead();
    void readyReadStandardOutput();
    void readyReadStandardError();
    void closed(int exitStatus);
    void x11ForwardingRequested(const QString &display);

private:
    SshRemoteProcessPrivate(const QByteArray &command, quint32 channelId,
        SshSendFacility &sendFacility, SshRemoteProcess *proc);
    SshRemoteProcessPrivate(quint32 channelId, SshSendFacility &sendFacility,
        SshRemoteProcess *proc);

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void handleOpenSuccessInternal();
    virtual void handleOpenFailureInternal(const QString &reason);
    virtual void handleChannelDataInternal(const QByteArray &data);
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data);
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus);
    virtual void handleExitSignal(const SshChannelExitSignal &signal);

    virtual void closeHook();

    void init();
    void setProcState(ProcessState newState);
    QByteArray &data();

    QProcess::ProcessChannel m_readChannel;

    ProcessState m_procState;
    bool m_wasRunning;
    int m_signal;
    int m_exitCode;

    const QByteArray m_command;
    const bool m_isShell;

    typedef QPair<QByteArray, QByteArray> EnvVar;
    QList<EnvVar> m_env;
    bool m_useTerminal;
    SshPseudoTerminal m_terminal;

    QString m_x11DisplayName;

    QByteArray m_stdout;
    QByteArray m_stderr;

    SshRemoteProcess *m_proc;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHREMOTEPROCESS_P_H
