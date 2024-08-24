#ifndef IKEV2_VPN_PROTOCOL_LINUX_H
#define IKEV2_VPN_PROTOCOL_LINUX_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

class Ikev2Protocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit Ikev2Protocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~Ikev2Protocol() override;

    ErrorCode start() override;
    void stop() override;

    static QString tunnelName() { return "AmneziaVPN IKEv2"; }


private:
    void readIkev2Configuration(const QJsonObject &configuration);

private:
    QJsonObject m_config;
    QString m_remoteAddress;
    int m_routeMode;


    bool create_new_vpn(const QString & vpn_name,
                        const QString & serv_addr);
    bool delete_vpn_connection(const QString &vpn_name);

    bool connect_to_vpn(const QString & vpn_name);
    bool disconnect_vpn();
};


#endif // IKEV2_VPN_PROTOCOL_LINUX_H
