#ifndef VPNPROTOCOL_H
#define VPNPROTOCOL_H

#include <QObject>
#include <QString>

#include "core/defs.h"
using namespace amnezia;

class QTimer;
class Communicator;

class VpnProtocol : public QObject
{
    Q_OBJECT

public:
    explicit VpnProtocol(const QString& args = QString(), QObject* parent = nullptr);
    virtual ~VpnProtocol() override = default;

    enum class ConnectionState {Unknown, Disconnected, Preparing, Connecting, Connected, Disconnecting, TunnelReconnecting, Error};

    static Communicator* communicator();
    static QString textConnectionState(ConnectionState connectionState);
    //static void initializeCommunicator(QObject* parent = nullptr);


    virtual bool onConnected() const;
    virtual bool onDisconnected() const;
    virtual ErrorCode start() = 0;
    virtual void stop() = 0;

    ConnectionState connectionState() const;
    ErrorCode lastError() const;
    QString textConnectionState() const;
    void setLastError(ErrorCode lastError);

    QString routeGateway() const;
    QString vpnGateway() const;

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(VpnProtocol::ConnectionState state);
    void timeoutTimerEvent();
    void protocolError(amnezia::ErrorCode e);

protected slots:
    virtual void onTimeout();

protected:
    void startTimeoutTimer();
    void stopTimeoutTimer();

    virtual void setBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    virtual void setConnectionState(VpnProtocol::ConnectionState state);

    //static Communicator* m_communicator;

    ConnectionState m_connectionState;
    QString m_routeGateway;
    QString m_vpnGateway;

private:
    QTimer* m_timeoutTimer;
    ErrorCode m_lastError;
    quint64 m_receivedBytes;
    quint64 m_sentBytes;

};

#endif // VPNPROTOCOL_H
