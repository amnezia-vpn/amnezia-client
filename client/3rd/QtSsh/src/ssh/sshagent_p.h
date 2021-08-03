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
#include <QHash>
#include <QList>
#include <QLocalSocket>
#include <QObject>
#include <QPair>
#include <QQueue>
#include <QString>

namespace QSsh {
namespace Internal {

class SshAgent : public QObject
{
    Q_OBJECT
public:
    enum State { Unconnected, Connecting, Connected, };

    ~SshAgent();
    static State state() { return instance().m_state; }
    static bool hasError() { return !instance().m_error.isEmpty(); }
    static QString errorString() { return instance().m_error; }
    static QList<QByteArray> publicKeys() { return instance().m_keys; }

    static void refreshKeys() { instance().refreshKeysImpl(); }
    static void storeDataToSign(const QByteArray &key, const QByteArray &data, uint token);
    static void removeDataToSign(const QByteArray &key, uint token);
    static void requestSignature(const QByteArray &key, uint token) {
        instance().requestSignatureImpl(key, token);
    }

    static SshAgent &instance();

signals:
    void errorOccurred();
    void keysUpdated();

    // Empty signature means signing failure.
    void signatureAvailable(const QByteArray &key, const QByteArray &signature, uint token);

private:
    struct Request {
        Request() { }
        Request(const QByteArray &k, const QByteArray &d, uint t)
            : key(k), dataToSign(d), token(t) { }

        bool isKeysRequest() const { return !isSignatureRequest(); }
        bool isSignatureRequest() const { return !key.isEmpty(); }

        QByteArray key;
        QByteArray dataToSign;
        uint token = 0;
    };

    struct Packet {
        bool isComplete() const { return size != 0 && int(size) == data.count(); }
        void invalidate() { size = 0; data.clear(); }

        quint32 size = 0;
        QByteArray data;
    };

    SshAgent();
    void connectToServer();
    void refreshKeysImpl();
    void requestSignatureImpl(const QByteArray &key, uint token);

    void sendNextRequest();
    Packet generateKeysPacket();
    Packet generateSigPacket(const Request &request);

    void handleConnected();
    void handleDisconnected();
    void handleSocketError();
    void handleIncomingData();
    void handleIncomingPacket();
    void handleIdentitiesPacket();
    void handleSignaturePacket();

    void handleProtocolError();
    void setDisconnected();

    void sendPacket();

    State m_state = Unconnected;
    QString m_error;
    QList<QByteArray> m_keys;
    QHash<QPair<QByteArray, uint>, QByteArray> m_dataToSign;
    QLocalSocket m_agentSocket;
    QByteArray m_incomingData;
    Packet m_incomingPacket;
    Packet m_outgoingPacket;

    QQueue<Request> m_pendingRequests;
};

} // namespace Internal
} // namespace QSsh
