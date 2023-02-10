#ifndef V2RAYPROTOCOL_H
#define V2RAYPROTOCOL_H

#include "QProcess"
#include "QTemporaryFile"

#include "openvpnprotocol.h"

class V2RayProtocol : public OpenVpnProtocol
{
public:
    V2RayProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~V2RayProtocol() override;

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

#endif // V2RAYPROTOCOL_H
