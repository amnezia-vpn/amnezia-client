#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"

#include "core/ipcclient.h"
#include "vpnprotocol.h"
#include "settings.h"

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
    Settings::RouteMode m_routeMode;
    QString m_primaryDNS;
    QString m_secondaryDNS;
#ifndef Q_OS_IOS
    QSharedPointer<IpcProcessTun2SocksReplica> m_t2sProcess;
#endif
};

#endif // XRAYPROTOCOL_H
