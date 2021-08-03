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

#ifndef SSHCHANNEL_P_H
#define SSHCHANNEL_P_H

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
    void handleWindowAdjust(quint64 bytesToAdd);
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
    void checkChannelActive() const;

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

#endif // SSHCHANNEL_P_H
