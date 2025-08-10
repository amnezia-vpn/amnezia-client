#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"

#include "containers/containers_defs.h"
#include "openvpnprotocol.h"
#include "settings.h"

class XrayProtocol : public VpnProtocol
{
public:
    XrayProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~XrayProtocol() override;

    ErrorCode start() override;
    ErrorCode startTun2Sock();
    void stop() override;

protected:
    void readXrayConfiguration(const QJsonObject &configuration);

protected:
    QJsonObject m_xrayConfig;

private:
    static QString xrayExecPath();
    static QString tun2SocksExecPath();

private:
    int m_localPort;
    QString m_remoteHost;
    QString m_remoteAddress;
    Settings::RouteMode m_routeMode;
    QJsonObject m_configData;
    QString m_primaryDNS;
    QString m_secondaryDNS;
#ifndef Q_OS_IOS
    QProcess m_xrayProcess;
    QSharedPointer<IpcProcessTun2SocksReplica> m_t2sProcess;
#endif
    QTemporaryFile m_xrayCfgFile;
};

#endif // XRAYPROTOCOL_H
