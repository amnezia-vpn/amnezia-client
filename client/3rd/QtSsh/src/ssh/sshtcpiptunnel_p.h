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

#pragma once

#include "sshchannel_p.h"
#include <QIODevice>
#include <QByteArray>

namespace QSsh {
namespace Internal {

class SshTcpIpTunnelPrivate : public AbstractSshChannel
{
    Q_OBJECT

public:
    SshTcpIpTunnelPrivate(quint32 channelId, SshSendFacility &sendFacility);
    ~SshTcpIpTunnelPrivate();

    template<class SshTcpIpTunnel>
    void init(SshTcpIpTunnel *q)
    {
        connect(this, &SshTcpIpTunnelPrivate::closed,
                q, &SshTcpIpTunnel::close, Qt::QueuedConnection);
        connect(this, &SshTcpIpTunnelPrivate::readyRead,
                q, &SshTcpIpTunnel::readyRead, Qt::QueuedConnection);
        connect(this, &SshTcpIpTunnelPrivate::error, q, [q](const QString &reason) {
            q->setErrorString(reason);
            emit q->error(reason);
        }, Qt::QueuedConnection);
    }

    void handleChannelSuccess() override;
    void handleChannelFailure() override;

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

signals:
    void readyRead();
    void error(const QString &reason);
    void closed();

protected:
    void handleOpenFailureInternal(const QString &reason) override;
    void handleChannelDataInternal(const QByteArray &data) override;
    void handleChannelExtendedDataInternal(quint32 type, const QByteArray &data) override;
    void handleExitStatus(const SshChannelExitStatus &exitStatus) override;
    void handleExitSignal(const SshChannelExitSignal &signal) override;
    void closeHook() override;

    QByteArray m_data;

private:
    void handleEof();
};

} // namespace Internal
} // namespace QSsh
