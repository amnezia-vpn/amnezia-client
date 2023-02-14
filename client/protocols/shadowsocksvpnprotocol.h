#ifndef SHADOWSOCKSVPNPROTOCOL_H
#define SHADOWSOCKSVPNPROTOCOL_H

#include "QProcess"

#include "openvpnprotocol.h"

class ShadowSocksVpnProtocol : public OpenVpnProtocol
{
public:
    ShadowSocksVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~ShadowSocksVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

protected:
    QJsonObject m_shadowSocksConfig;

    void readShadowSocksConfiguration(const QJsonObject &configuration);

private:
#ifndef Q_OS_IOS
    QProcess m_ssProcess;
#endif
    QTemporaryFile m_shadowSocksCfgFile;

    static QString shadowSocksExecPath();
};

#endif // SHADOWSOCKSVPNPROTOCOL_H
