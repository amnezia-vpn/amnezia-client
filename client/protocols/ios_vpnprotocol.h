#ifndef IOS_VPNPROTOCOL_H
#define IOS_VPNPROTOCOL_H

#include "platforms/ios/json.h"
#include "vpnprotocol.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;


class IOSVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit IOSVpnProtocol(amnezia::Proto proto, const QJsonObject& configuration, QObject* parent = nullptr);
    static IOSVpnProtocol* instance();
    
    virtual ~IOSVpnProtocol() override = default;

    bool initialize();

    virtual ErrorCode start() override;
    virtual void stop() override;

    void resume_start();

    void checkStatus();

    void setNotificationText(const QString& title, const QString& message,
                             int timerSec);
    void setFallbackConnectedNotification();

    void getBackendLogs(std::function<void(const QString&)>&& callback);

    void cleanupBackendLogs();

signals:

protected slots:

protected:

private:
    Proto m_protocol;
    bool m_serviceConnected = false;
    bool m_checkingStatus = false;
    std::function<void(const QString&)> m_logCallback;
    
    bool m_isChangingState = false;
    
    void setupWireguardProtocol(const QtJson::JsonObject& result);
    void setupOpenVPNProtocol(const QtJson::JsonObject& result);
    void setupShadowSocksProtocol(const QtJson::JsonObject& result);
    
    void launchWireguardTunnel(const QtJson::JsonObject &result);
    void launchOpenVPNTunnel(const QtJson::JsonObject &result);
    void launchShadowSocksTunnel(const QtJson::JsonObject &result);
    
    QString serializeSSConfig(const QtJson::JsonObject &ssConfig);
};


#endif // IOS_VPNPROTOCOL_H
