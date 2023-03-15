#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QRemoteObjectNode>

#include "protocols/vpnprotocol.h"
#include "core/defs.h"
#include "settings.h"

#ifdef Q_OS_IOS
#include "protocols/ios_vpnprotocol.h"
#endif

#ifdef AMNEZIA_DESKTOP
#include "core/ipcclient.h"
#endif

#ifdef Q_OS_ANDROID
#include "protocols/android_vpnprotocol.h"
#endif

class VpnConfigurator;
class ServerController;

using namespace amnezia;

class VpnConnection : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnection(std::shared_ptr<Settings> settings,
        std::shared_ptr<VpnConfigurator> configurator,
        std::shared_ptr<ServerController> serverController, QObject* parent = nullptr);
    ~VpnConnection() override;

    static QString bytesPerSecToText(quint64 bytes);

    ErrorCode lastError() const;

    static QMap<Proto, QString> getLastVpnConfig(const QJsonObject &containerConfig);
    QString createVpnConfigurationForProto(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig, Proto proto,
        ErrorCode *errorCode = nullptr);

    QJsonObject createVpnConfiguration(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);



    bool isConnected() const;
    bool isDisconnected() const;

    VpnProtocol::VpnConnectionState connectionState();
    QSharedPointer<VpnProtocol> vpnProtocol() const;

    void addRoutes(const QStringList &ips);
    void deleteRoutes(const QStringList &ips);
    void flushDns();

    const QString &remoteAddress() const;
    void addSitesRoutes(const QString &gw, Settings::RouteMode mode);

#ifdef Q_OS_ANDROID
    void restoreConnection();
#endif

public slots:
    void connectToVpn(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);

    void disconnectFromVpn();

signals:
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void connectionStateChanged(VpnProtocol::VpnConnectionState state);
    void vpnProtocolError(amnezia::ErrorCode error);

    void serviceIsNotReady();

protected slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::VpnConnectionState state);

#ifdef Q_OS_IOS
    void checkIOSStatus();
#endif

protected:
    QSharedPointer<VpnProtocol> m_vpnProtocol;

private:
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;
    std::shared_ptr<ServerController> m_serverController;

    QJsonObject m_vpnConfiguration;
    QJsonObject m_routeMode;
    QString m_remoteAddress;
    bool m_isIOSConnected;  //remove later move to isConnected,

#ifdef AMNEZIA_DESKTOP
    IpcClient *m_IpcClient {nullptr};
#endif
#ifdef Q_OS_IOS
    IOSVpnProtocol * iosVpnProtocol{nullptr};
#endif
#ifdef Q_OS_ANDROID
   AndroidVpnProtocol* androidVpnProtocol = nullptr;
   AndroidVpnProtocol* createDefaultAndroidVpnProtocol(DockerContainer container);

   void createAndroidConnections();
   void createAndroidConnections(DockerContainer container);
#endif

   void createProtocolConnections();
};

#endif // VPNCONNECTION_H
