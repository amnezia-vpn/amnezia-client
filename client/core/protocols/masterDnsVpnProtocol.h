// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MASTERDNSVPNPROTOCOL_H
#define MASTERDNSVPNPROTOCOL_H

#include <QHostAddress>
#include <QList>
#include <QSharedPointer>
#include <QString>

#include "core/utils/commonStructs.h"
#include "core/utils/errorCodes.h"
#include "core/utils/ipcClient.h"
#include "core/utils/routeModes.h"
#include "vpnProtocol.h"

class MasterDnsVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    MasterDnsVpnProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    ~MasterDnsVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    ErrorCode startTun2Socks(quint16 socksPort);
    ErrorCode setupRouting();

    QJsonObject m_engineConfig;
    amnezia::RouteMode m_routeMode = amnezia::RouteMode::VpnAllSites;
    QList<QHostAddress> m_dnsServers;
    QString m_remoteAddress;

    QSharedPointer<IpcProcessInterfaceReplica> m_tun2socksProcess;
};

#endif // MASTERDNSVPNPROTOCOL_H
