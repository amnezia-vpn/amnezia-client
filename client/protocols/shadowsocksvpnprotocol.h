#ifndef SHADOWSOCKSVPNPROTOCOL_H
#define SHADOWSOCKSVPNPROTOCOL_H

#include "openvpnprotocol.h"
#include "QProcess"

class ShadowSocksVpnProtocol : public OpenVpnProtocol
{
public:
    ShadowSocksVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);

    ErrorCode start() override;
    void stop() override;

    static QJsonObject genShadowSocksConfig(const ServerCredentials &credentials, Protocol proto = Protocol::ShadowSocks);

protected:
    void readShadowSocksConfiguration(const QJsonObject &configuration);
    QString shadowSocksExecPath() const;

protected:
    QJsonObject m_shadowSocksConfig;

private:
    QProcess m_ssProcess;
};

#endif // SHADOWSOCKSVPNPROTOCOL_H
