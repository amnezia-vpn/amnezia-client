#ifndef SHADOWSOCKSVPNPROTOCOL_H
#define SHADOWSOCKSVPNPROTOCOL_H

#include "openvpnprotocol.h"
#include "QProcess"
#include "containers/containers_defs.h"

class ShadowSocksVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    ShadowSocksVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~ShadowSocksVpnProtocol() override;


    ErrorCode startTun2Sock();
    ErrorCode start() override;
    void stop() override;

protected:
    void readShadowSocksConfiguration(const QJsonObject &configuration);

protected:
    QJsonObject m_shadowSocksConfig;
    int m_localPort;

private:
    static QString shadowSocksExecPath();
    static QString tun2SocksExecPath();

private:
#ifndef Q_OS_IOS
    QProcess m_ssProcess;
    QProcess m_t2sProcess;
#endif
    QTemporaryFile m_shadowSocksCfgFile;
};

#endif // SHADOWSOCKSVPNPROTOCOL_H
