#ifndef SHADOWSOCKSVPNPROTOCOL_H
#define SHADOWSOCKSVPNPROTOCOL_H

#include "openvpnprotocol.h"
#include "QProcess"
#include "containers/containers_defs.h"

#define Q_OS_IOS 1

class ShadowSocksVpnProtocol : public OpenVpnProtocol
{
public:
    ShadowSocksVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~ShadowSocksVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

protected:
    void readShadowSocksConfiguration(const QJsonObject &configuration);

protected:
    QJsonObject m_shadowSocksConfig;

private:
    static QString shadowSocksExecPath();

private:
#ifndef Q_OS_IOS
    QProcess m_ssProcess;
#endif
//    QTemporaryFile m_shadowSocksCfgFile;
};

#endif // SHADOWSOCKSVPNPROTOCOL_H
