#ifndef VPNTRAFFICGUARD_H
#define VPNTRAFFICGUARD_H

#include <qobject.h>
#include "core/repositories/secureAppSettingsRepository.h"
#include "protocols/vpnProtocol.h"

class VpnTrafficGuard : public QObject
{
    Q_OBJECT

public:
    explicit VpnTrafficGuard(SecureAppSettingsRepository* appSettings, QObject* parent = nullptr);
    ~VpnTrafficGuard() override;
    void setConfig(const QJsonObject &config);
    void setupRoutes(const QJsonObject &vpnConfiguration,
                                 const QSharedPointer<VpnProtocol> &protocol,
                                 const QString &remoteAddress);

    void teardown();
    bool allowEndpoint(const QString &remoteAddress);
    void applyFirewall(const QString &vpnGateway, const QString &vpnLocalAddress);
private:
    void addSplitTunnelRoutes(const QString &gateway, amnezia::RouteMode mode);
    SecureAppSettingsRepository* m_appSettingsRepository;
    QJsonObject m_config;
    bool m_ipv6RoutingStopped = false;

};

#endif // VPNTRAFFICGUARD_H
