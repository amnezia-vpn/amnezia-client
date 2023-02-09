#ifndef V2RAYPROTOCOL_H
#define V2RAYPROTOCOL_H

#include "vpnprotocol.h"

class V2RayProtocol : public VpnProtocol
{
public:
    V2RayProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~V2RayProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    QJsonObject m_shadowSocksConfig;
    void writeV2RayConfiguration(const QJsonObject &configuration);

    const QString v2rayExecPath() const;
};

#endif // V2RAYPROTOCOL_H
