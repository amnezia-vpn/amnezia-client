/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sshx11displayinfo_p.h"

#include <QByteArray>

namespace QSsh {
namespace Internal {
class SshChannelManager;
class SshSendFacility;
class X11Socket;

class SshX11Channel : public AbstractSshChannel
{
    Q_OBJECT

    friend class Internal::SshChannelManager;

signals:
    void error(const QString &message);

private:
    SshX11Channel(const X11DisplayInfo &displayInfo, quint32 channelId,
                  SshSendFacility &sendFacility);

    void handleChannelSuccess() override;
    void handleChannelFailure() override;

    void handleOpenSuccessInternal() override;
    void handleOpenFailureInternal(const QString &reason) override;
    void handleChannelDataInternal(const QByteArray &data) override;
    void handleChannelExtendedDataInternal(quint32 type, const QByteArray &data) override;
    void handleExitStatus(const SshChannelExitStatus &exitStatus) override;
    void handleExitSignal(const SshChannelExitSignal &signal) override;
    void closeHook() override;

    void handleRemoteData(const QByteArray &data);

    X11Socket * const m_x11Socket;
    const X11DisplayInfo m_displayInfo;
    QByteArray m_queuedRemoteData;
    bool m_haveReplacedRandomCookie = false;
};

} // namespace Internal
} // namespace QSsh
