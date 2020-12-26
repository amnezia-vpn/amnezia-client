#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QString>
#include <QScopedPointer>

#include "vpnprotocol.h"

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(QObject* parent = nullptr);
    ~VpnConnection();

    enum class Protocol{OpenVpn};
    static QString bytesToText(quint64 bytes);

    QString lastError() const;
    bool connectToVpn(Protocol protocol = Protocol::OpenVpn);
    bool connected() const;
    bool disconnected() const;
    void disconnectFromVpn();

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(VpnProtocol::ConnectionState state);

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);

protected:

    QScopedPointer<VpnProtocol> m_vpnProtocol;
};

#endif // VPNCONNECTION_H
