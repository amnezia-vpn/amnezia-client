#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QRemoteObjectNode>
#include <QTimer>

#include "protocols/vpnprotocol.h"
#include "core/defs.h"
#include "settings.h"

#ifdef AMNEZIA_DESKTOP
#include "core/ipcclient.h"
#endif

#ifdef Q_OS_ANDROID
#include "protocols/android_vpnprotocol.h"
#endif

using namespace amnezia;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(std::shared_ptr<Settings> settings, QObject* parent = nullptr);
    ~VpnConnection() override;

    static QString bytesPerSecToText(quint64 bytes);

    ErrorCode lastError() const;

    bool isConnected() const;
    bool isDisconnected() const;

    Vpn::ConnectionState connectionState();
    QSharedPointer<VpnProtocol> vpnProtocol() const;

    const QString &remoteAddress() const;
    void addSitesRoutes(const QString &gw, Settings::RouteMode mode);

#ifdef Q_OS_ANDROID
    void restoreConnection();
#endif

public slots:
    void connectToVpn(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &vpnConfiguration);

    void disconnectFromVpn();


    void addRoutes(const QStringList &ips);
    void deleteRoutes(const QStringList &ips);
    void flushDns();

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(Vpn::ConnectionState state);
    void vpnProtocolError(amnezia::ErrorCode error);

    void serviceIsNotReady();

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(Vpn::ConnectionState state);

protected:
    QSharedPointer<VpnProtocol> m_vpnProtocol;

private:
    std::shared_ptr<Settings> m_settings;
    QJsonObject m_vpnConfiguration;
    QJsonObject m_routeMode;
    QString m_remoteAddress;

    // Only for iOS for now, check counters
    QTimer m_checkTimer;

#ifdef AMNEZIA_DESKTOP
    IpcClient *m_IpcClient {nullptr};
#endif

#ifdef Q_OS_ANDROID
   AndroidVpnProtocol* androidVpnProtocol = nullptr;

   AndroidVpnProtocol* createDefaultAndroidVpnProtocol();
   void createAndroidConnections();
#endif

   void createProtocolConnections();

   void appendSplitTunnelingConfig();
};

#endif // VPNCONNECTION_H
