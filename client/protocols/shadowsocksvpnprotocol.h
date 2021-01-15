#ifndef SHADOWSOCKSVPNPROTOCOL_H
#define SHADOWSOCKSVPNPROTOCOL_H

#include "openvpnprotocol.h"
#include "QProcess"

class ShadowSocksVpnProtocol : public OpenVpnProtocol
{
public:
    ShadowSocksVpnProtocol(const QString& args = QString(), QObject* parent = nullptr);

    ErrorCode start() override;
    void stop() override;

    static QString genShadowSocksConfig(const ServerCredentials &credentials, Protocol proto = Protocol::ShadowSocks);

protected:
    QString shadowSocksExecPath() const;

protected:
    QString m_shadowSocksConfig;

private:
    QProcess ssProcess;
};

#endif // SHADOWSOCKSVPNPROTOCOL_H
