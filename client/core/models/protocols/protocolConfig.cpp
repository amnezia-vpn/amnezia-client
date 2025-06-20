#include "protocolConfig.h"

#include <QJsonObject>

ProtocolConfig::ProtocolConfig(const QString &protocolName) : protocolName(protocolName)
{
}

QJsonObject ProtocolConfig::toJson() const
{
    return QJsonObject();
}
