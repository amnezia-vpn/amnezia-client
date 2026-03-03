#ifndef HYSTERIA2PROTOCOL_H
#define HYSTERIA2PROTOCOL_H

#include "QProcess"

#include "core/ipcclient.h"
#include "vpnprotocol.h"
#include "settings.h"
#include <QtCore/qsharedpointer.h>

class Hysteria2Protocol : public VpnProtocol
{
public:
    Hysteria2Protocol(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~Hysteria2Protocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    ErrorCode setupRouting();
    ErrorCode startTun2Socks();

    QString buildHysteria2Config() const;

    Settings::RouteMode m_routeMode;
    QList<QHostAddress> m_dnsServers;
    QString m_remoteAddress;
    int m_socksPort;

    // Hysteria2 connection parameters (extracted from raw config)
    QString m_server;
    int m_serverPort;
    QString m_password;
    QString m_obfs;
    QString m_obfsPassword;
    QString m_sni;
    bool m_insecure;
    int m_upMbps;
    int m_downMbps;

    QSharedPointer<IpcProcessInterfaceReplica> m_tun2socksProcess;
};

#endif // HYSTERIA2PROTOCOL_H
