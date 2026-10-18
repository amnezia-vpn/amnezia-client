#ifndef XRAYCONFIGUTILS_H
#define XRAYCONFIGUTILS_H

#include <optional>

#include <QJsonObject>
#include <QString>

namespace xrayConfigUtils
{
    std::optional<QString> extractLastConfig(const QJsonObject &serverConfig);
    bool setInboundPort(QJsonObject &xrayConfig, int port);
}

#endif // XRAYCONFIGUTILS_H
