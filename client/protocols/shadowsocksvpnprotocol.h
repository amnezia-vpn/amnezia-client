#ifndef SHADOWSOCKSVPNPROTOCOL_H
#define SHADOWSOCKSVPNPROTOCOL_H

#include "openvpnprotocol.h"
#include "QProcess"

class ShadowSocksVpnProtocol : public OpenVpnProtocol
{
public:
    ShadowSocksVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~ShadowSocksVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

    static QJsonObject genShadowSocksConfig(const ServerCredentials &credentials, Protocol proto = Protocol::ShadowSocks);

protected:
    void readShadowSocksConfiguration(const QJsonObject &configuration);

protected:
    QJsonObject m_shadowSocksConfig;

private:
    static QString shadowSocksExecPath();

private:
    QProcess m_ssProcess;
    QTemporaryFile m_shadowSocksCfgFile;
};

#endif // SHADOWSOCKSVPNPROTOCOL_H
