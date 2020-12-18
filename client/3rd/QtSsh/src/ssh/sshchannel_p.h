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

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>

namespace QSsh {
namespace Internal {

struct SshChannelExitSignal;
struct SshChannelExitStatus;
class SshIncomingPacket;
class SshSendFacility;

class AbstractSshChannel : public QObject
{
    Q_OBJECT
public:
    enum ChannelState {
        Inactive, SessionRequested, SessionEstablished, CloseRequested, Closed
    };

    quint32 localChannelId() const { return m_localChannel; }
    quint32 remoteChannel() const { return m_remoteChannel; }

    virtual void handleChannelSuccess() = 0;
    virtual void handleChannelFailure() = 0;

    void handleOpenSuccess(quint32 remoteChannelId, quint32 remoteWindowSize,
        quint32 remoteMaxPacketSize);
    void handleOpenFailure(const QString &reason);
    void handleWindowAdjust(quint32 bytesToAdd);
    void handleChannelEof();
    void handleChannelClose();
    void handleChannelData(const QByteArray &data);
    void handleChannelExtendedData(quint32 type, const QByteArray &data);
    void handleChannelRequest(const SshIncomingPacket &packet);

    void closeChannel();

    virtual ~AbstractSshChannel();

    static const int ReplyTimeout = 10000; // milli seconds
    ChannelState channelState() const { return m_state; }

signals:
    void timeout();
    void eof();

protected:
    AbstractSshChannel(quint32 channelId, SshSendFacility &sendFacility);

    void setChannelState(ChannelState state);

    void requestSessionStart();
    void sendData(const QByteArray &data);

    static quint32 initialWindowSize();
    static quint32 maxPacketSize();

    quint32 maxDataSize() const;
    void checkChannelActive();

    SshSendFacility &m_sendFacility;
    QTimer m_timeoutTimer;

private:
    virtual void handleOpenSuccessInternal() = 0;
    virtual void handleOpenFailureInternal(const QString &reason) = 0;
    virtual void handleChannelDataInternal(const QByteArray &data) = 0;
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data) = 0;
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus) = 0;
    virtual void handleExitSignal(const SshChannelExitSignal &signal) = 0;

    virtual void closeHook() = 0;

    void flushSendBuffer();
    int handleChannelOrExtendedChannelData(const QByteArray &data);

    const quint32 m_localChannel;
    quint32 m_remoteChannel;
    quint32 m_localWindowSize;
    quint32 m_remoteWindowSize;
    quint32 m_remoteMaxPacketSize;
    ChannelState m_state;
    QByteArray m_sendBuffer;
};

} // namespace Internal
} // namespace QSsh
