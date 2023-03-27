#ifndef V2RAYTROJANPROTOCOL_H
#define V2RAYTROJANPROTOCOL_H

#include "QProcess"
#include "QTemporaryFile"

#include "openvpnprotocol.h"

class V2RayTrojanProtocol : public OpenVpnProtocol
{
public:
    V2RayTrojanProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~V2RayTrojanProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    QJsonObject m_v2RayConfig;
    QTemporaryFile m_v2RayConfigFile;
#ifndef Q_OS_IOS
    QProcess m_v2RayProcess;
#endif

    void writeV2RayConfiguration(const QJsonObject &configuration);

    const QString v2RayExecPath() const;
};

#endif // V2RAYTROJANPROTOCOL_H
