#ifndef VPNPROTOCOL_H
#define VPNPROTOCOL_H

#include <QObject>
#include <QString>

class QTimer;
class Communicator;

class VpnProtocol : public QObject
{
    Q_OBJECT

public:
    explicit VpnProtocol(const QString& args = QString(), QObject* parent = nullptr);
    ~VpnProtocol();

    enum class ConnectionState {Unknown, Disconnected, Preparing, Connecting, Connected, Disconnecting, TunnelReconnecting, Error};
    static QString textConnectionState(ConnectionState connectionState);

    ConnectionState connectionState() const;
    QString textConnectionState() const;
    virtual bool connected() const;
    virtual bool disconnected() const;
    virtual bool start() = 0;
    virtual void stop() = 0;

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(VpnProtocol::ConnectionState state);
    void timeoutTimerEvent();

protected slots:
    virtual void onTimeout();

protected:
    void startTimeoutTimer();
    void stopTimeoutTimer();

    virtual void setBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    virtual void setConnectionState(VpnProtocol::ConnectionState state);

    Communicator* m_communicator;
    ConnectionState m_connectionState;
    QTimer* m_timeoutTimer;
};

#endif // VPNPROTOCOL_H
