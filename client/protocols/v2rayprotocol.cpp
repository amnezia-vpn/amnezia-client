#include "v2rayprotocol.h"

#include "utilities.h"

V2RayProtocol::V2RayProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    writeV2RayConfiguration(configuration);
}

V2RayProtocol::~V2RayProtocol()
{

}

ErrorCode V2RayProtocol::start()
{
    return ErrorCode::NoError;
}

void V2RayProtocol::stop()
{

}

void V2RayProtocol::writeV2RayConfiguration(const QJsonObject &configuration)
{

}

const QString V2RayProtocol::v2rayExecPath() const
{
#ifdef Q_OS_WIN
    return Utils::executable(QString("v2ray/v2ray"), true);
#else
    return Utils::executable(QString("/v2ray/v2ray"), true);
#endif
}
