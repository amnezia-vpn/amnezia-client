#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QRemoteObjectNode>

#include "protocols/vpnprotocol.h"
#include "core/defs.h"
#include "core/ipcclient.h"
#include "settings.h"

using namespace amnezia;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(QObject* parent = nullptr);
    ~VpnConnection() override;

    static QString bytesPerSecToText(quint64 bytes);

    ErrorCode lastError() const;

    static QMap<Protocol, QString> getLastVpnConfig(const QJsonObject &containerConfig);
    QString createVpnConfigurationForProto(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig, Protocol proto,
        ErrorCode *errorCode = nullptr);

    ErrorCode createVpnConfiguration(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);

    ErrorCode connectToVpn(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);

    void disconnectFromVpn();

    bool isConnected() const;
    bool isDisconnected() const;

    VpnProtocol::ConnectionState connectionState();
    QSharedPointer<VpnProtocol> vpnProtocol() const;

    void addRoutes(const QStringList &ips);
    void deleteRoutes(const QStringList &ips);
    void flushDns();

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(VpnProtocol::ConnectionState state);
    void vpnProtocolError(amnezia::ErrorCode error);

    void serviceIsNotReady();

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);

protected:
    QSharedPointer<VpnProtocol> m_vpnProtocol;

private:
    Settings m_settings;
    QJsonObject m_vpnConfiguration;
    QJsonObject m_routeMode;

};

#endif // VPNCONNECTION_H
