#ifndef VPNPROTOCOL_H
#define VPNPROTOCOL_H

#include <QObject>
#include <QString>
#include <QJsonObject>

#include "core/defs.h"
#include "containers/containers_defs.h"

using namespace amnezia;

class QTimer;

//todo change name
namespace Vpn
{
    Q_NAMESPACE
    enum ConnectionState {
        Unknown,
        Disconnected,
        Preparing,
        Connecting,
        Connected,
        Disconnecting,
        Reconnecting,
        Error
    };
    Q_ENUM_NS(ConnectionState)

    static void declareQmlVpnConnectionStateEnum() {
        qmlRegisterUncreatableMetaObject(
            Vpn::staticMetaObject,
            "ConnectionState",
            1, 0,
            "ConnectionState",
            "Error: only enums"
            );
    }
}

class VpnProtocol : public QObject
{
    Q_OBJECT

public:
    explicit VpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~VpnProtocol() override = default;

    static QString textConnectionState(Vpn::ConnectionState connectionState);

    virtual ErrorCode prepare() { return ErrorCode::NoError; }

    virtual bool isConnected() const;
    virtual bool isDisconnected() const;
    virtual ErrorCode start() = 0;
    virtual void stop() = 0;

    Vpn::ConnectionState connectionState() const;
    ErrorCode lastError() const;
    QString textConnectionState() const;
    void setLastError(ErrorCode lastError);

    QString routeGateway() const;
    QString vpnGateway() const;

    static VpnProtocol* factory(amnezia::DockerContainer container, const QJsonObject &configuration);

    virtual void waitForDisconected(int msecs) {}

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(Vpn::ConnectionState state);
    void timeoutTimerEvent();
    void protocolError(amnezia::ErrorCode e);

    void newRoute(const QString& ip);
    void newDns(const QString& dnsAddr);
    void finishReceivingSettings();

public slots:
    virtual void onTimeout(); // todo: remove?

    void setBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void setConnectionState(Vpn::ConnectionState state);

protected:
    void startTimeoutTimer();
    void stopTimeoutTimer();

    Vpn::ConnectionState m_connectionState;

    QString m_routeGateway;
    QString m_vpnLocalAddress;
    QString m_vpnGateway;

    QJsonObject m_rawConfig;

private:
    QTimer* m_timeoutTimer;
    ErrorCode m_lastError;
    quint64 m_receivedBytes;
    quint64 m_sentBytes;
};

#endif // VPNPROTOCOL_H
