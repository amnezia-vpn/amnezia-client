#ifndef IOS_CONTROLLER_H
#define IOS_CONTROLLER_H

#include "protocols/vpnprotocol.h"
#include <functional>
#include <QVariant>
#include <QVariantMap>
#include <QStringList>
#include <QList>

#ifdef __OBJC__
    #import <Foundation/Foundation.h>
@class NETunnelProviderManager;
#endif

using namespace amnezia;

struct Action
{
    static const char *start;
    static const char *restart;
    static const char *stop;
    static const char *getTunnelId;
    static const char *getStatus;
};

struct MessageKey
{
    static const char *action;
    static const char *tunnelId;
    static const char *config;
    static const char *errorCode;
    static const char *host;
    static const char *port;
    static const char *isOnDemand;
    static const char *SplitTunnelType;
    static const char *SplitTunnelSites;
};

class IosController : public QObject
{
    Q_OBJECT

public:
    static IosController *Instance();

    virtual ~IosController() override = default;

    bool initialize();
    bool connectVpn(amnezia::Proto proto, const QJsonObject &configuration);
    void disconnectVpn();

    void vpnStatusDidChange(void *pNotification);
    
    void vpnConfigurationDidChange(void *pNotification);

    void getBackendLogs(std::function<void(const QString &)> &&callback);
    void checkStatus();

    bool shareText(const QStringList &filesToSend);
    QString openFile();

    void purchaseProduct(const QString &productId,
                         std::function<void(bool success,
                                            const QString &transactionId,
                                            const QString &purchasedProductId,
                                            const QString &originalTransactionId,
                                            const QString &errorString)> &&callback);
    void restorePurchases(std::function<void(bool success,
                                             const QList<QVariantMap> &transactions,
                                             const QString &errorString)> &&callback);

    // Fetch product info for given product identifiers and return basic fields for logging
    void fetchProducts(const QStringList &productIds,
                       std::function<void(const QList<QVariantMap> &products,
                                          const QStringList &invalidIds,
                                          const QString &errorString)> &&callback);

    void requestInetAccess();
    bool isTestFlight();
signals:
    void connectionStateChanged(Vpn::ConnectionState state);
    void bytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void importConfigFromOutside(const QString);
    void importBackupFromOutside(const QString);

    void finished();

protected slots:

private:
    explicit IosController();

    bool setupOpenVPN();
    bool setupCloak();
    bool setupWireGuard();
    bool setupAwg();
    bool setupXray();
    bool setupSSXray();

    bool startOpenVPN(const QString &config);
    bool startWireGuard(const QString &jsonConfig);
    bool startXray(const QString &jsonConfig);

    void startTunnel();

private:
    void *m_iosControllerWrapper {};
#ifdef __OBJC__
    NETunnelProviderManager *m_currentTunnel {};
    NSString *m_serverAddress {};
    bool isOurManager(NETunnelProviderManager *manager);
    void sendVpnExtensionMessage(NSDictionary *message, std::function<void(NSDictionary *)> callback = nullptr);
#endif

    amnezia::Proto m_proto;
    QJsonObject m_rawConfig;
    QString m_tunnelId;
    uint64_t m_txBytes;
    uint64_t m_rxBytes;
};

#endif // IOS_CONTROLLER_H
