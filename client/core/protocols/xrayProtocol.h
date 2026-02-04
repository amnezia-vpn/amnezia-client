#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"

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
    ErrorCode startTun2Sock();
    void readXrayConfiguration(const QJsonObject &configuration);
    
    QJsonObject m_xrayConfig;
    amnezia::RouteMode m_routeMode;
    QString m_primaryDNS;
    QString m_secondaryDNS;
#ifndef Q_OS_IOS
    QSharedPointer<IpcProcessTun2SocksReplica> m_t2sProcess;
#endif
};

#endif // XRAYPROTOCOL_H
