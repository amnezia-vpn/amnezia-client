#ifndef OPENVPNOVERCLOAKPROTOCOL_H
#define OPENVPNOVERCLOAKPROTOCOL_H

#include "openvpnprotocol.h"
#include "QProcess"

class OpenVpnOverCloakProtocol : public OpenVpnProtocol
{
public:
    OpenVpnOverCloakProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~OpenVpnOverCloakProtocol() override;

    ErrorCode start() override;
    void stop() override;

protected:
    void readCloakConfiguration(const QJsonObject &configuration);

protected:
    QJsonObject m_cloakConfig;

private:
    static QString cloakExecPath();

private:
    QProcess m_ckProcess;
    QTemporaryFile m_cloakCfgFile;
    QMetaObject::Connection m_errorHandlerConnection;
};

#endif // OPENVPNOVERCLOAKPROTOCOL_H
