#pragma once

#include <QObject>
#include <QTimer>


#include "openvpnprotocol.h"


class Ikev2Protocol : public VpnProtocol
{
    Q_OBJECT
public:
    explicit Ikev2Protocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~Ikev2Protocol() override;

    void readIkev2Configuration(const QJsonObject &configuration);
    bool create_new_vpn(const QString &vpn_name, const QString &serv_addr);
    bool delete_vpn_connection(const QString &vpn_name);
    bool connect_to_vpn(const QString & vpn_name);
    bool disconnect_vpn();
    void closeWindscribeActiveConnection();

    ErrorCode start() override;
    void stop() override;

    static QString tunnelName() { return "AmneziaVPN IKEv2"; }

private slots:
    void handleNotificationImpl(int status);

private:
    enum {STATE_DISCONNECTED, STATE_START_CONNECT, STATE_START_DISCONNECTING, STATE_CONNECTED, STATE_DISCONNECTING_AUTH_ERROR, STATE_DISCONNECTING_ANY_ERROR};

    int state_;

    bool bConnected_;
    mutable QRecursiveMutex mutex_;
    void *notificationId_;
    bool isStateConnectingAfterClick_;
    bool isDisconnectClicked_;

    QString overrideDnsIp_;

    QJsonObject m_config;

    static constexpr int STATISTICS_UPDATE_PERIOD = 1000;
    QTimer statisticsTimer_;
    QString ipsecAdapterName_;

    int prevConnectionStatus_;
    bool isPrevConnectionStatusInitialized_;

    // True if startConnect() method was called and NEVPNManager emitted notification NEVPNStatusConnecting.
    // False otherwise.
    bool isConnectingStateReachedAfterStartingConnection_;

    void handleNotification(void *notification);
    bool isFailedAuthError(QMap<time_t, QString> &logs);
    bool isSocketError(QMap<time_t, QString> &logs);
    bool setCustomDns(const QString &overrideDnsIpAddress);
};
