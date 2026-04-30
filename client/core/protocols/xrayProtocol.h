#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"
#include <QtCore/qsharedpointer.h>
#include <QHostAddress>
#include <QList>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/ipcClient.h"
#include "vpnProtocol.h"

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
    amnezia::RouteMode m_routeMode;
    QList<QHostAddress> m_dnsServers;
    QString m_remoteAddress;

    QString m_socksUser;
    QString m_socksPassword;
    int m_socksPort = 10808;

    QSharedPointer<IpcProcessInterfaceReplica> m_tun2socksProcess;
};

#endif // XRAYPROTOCOL_H
