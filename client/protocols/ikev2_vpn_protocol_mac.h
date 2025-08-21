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
    mutable QRecursiveMutex mutex_;
    void *notificationId_;
    QJsonObject m_config;
    QJsonObject m_ikev2_config;

    QString ipsecAdapterName_;
    
    bool isConnectingStateReachedAfterStartingConnection_;
    
    void handleNotification(void *notification);
    bool isFailedAuthError(QMap<time_t, QString> &logs);
    bool isSocketError(QMap<time_t, QString> &logs);
    bool setCustomDns(const QString &overrideDnsIpAddress);
};
