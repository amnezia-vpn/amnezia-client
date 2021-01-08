#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QString>
#include <QScopedPointer>

#include "protocols/vpnprotocol.h"
#include "core/defs.h"

using namespace amnezia;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(QObject* parent = nullptr);
    ~VpnConnection() override = default;

    static QString bytesToText(quint64 bytes);

    ErrorCode lastError() const;
    ErrorCode requestVpnConfig(const ServerCredentials &credentials, Protocol protocol);
    ErrorCode connectToVpn(const ServerCredentials &credentials, Protocol protocol = Protocol::Any);
    bool onConnected() const;
    bool onDisconnected() const;
    void disconnectFromVpn();

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(VpnProtocol::ConnectionState state);
    void vpnProtocolError(amnezia::ErrorCode error);

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);

protected:

    QScopedPointer<VpnProtocol> m_vpnProtocol;
};

#endif // VPNCONNECTION_H
