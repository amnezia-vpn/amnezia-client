#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"

#include "core/ipcclient.h"
#include "core/privileged_process.h"
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
    ErrorCode startTun2Sock();
    void readXrayConfiguration(const QJsonObject &configuration);
    
    QJsonObject m_xrayConfig;
    Settings::RouteMode m_routeMode;
    QString m_primaryDNS;
    QString m_secondaryDNS;
#ifdef AMNEZIA_DESKTOP
    QSharedPointer<PrivilegedProcess> m_tunProcess;
#endif
};

#endif // XRAYPROTOCOL_H
