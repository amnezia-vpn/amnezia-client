#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"

#include "core/ipcclient.h"
#include "vpnprotocol.h"
#include "settings.h"
#include <QtCore/qsharedpointer.h>

class XrayProtocol : public VpnProtocol
{
public:
    XrayProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~XrayProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    ErrorCode setupRouting();
    ErrorCode startTun2Socks();

    QJsonObject m_xrayConfig;
    Settings::RouteMode m_routeMode;
    QList<QHostAddress> m_dnsServers;
    QString m_remoteAddress;

    QSharedPointer<IpcProcessInterfaceReplica> m_tun2socksProcess;
};

#endif // XRAYPROTOCOL_H
