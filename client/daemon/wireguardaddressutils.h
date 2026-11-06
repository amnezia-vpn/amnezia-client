#pragma once

#include <QHostAddress>
#include <QStringList>
#include <optional>

namespace WireguardAddressUtils
{
    struct DeviceAddresses
    {
        QString ipv4;
        QString ipv6;
    };

    inline std::optional<DeviceAddresses> parseDeviceAddresses(const QString &value)
    {
        DeviceAddresses result;
        for (const QString &part : value.split(',')) {
            const QString subnet = part.trimmed();
            const auto parsed = QHostAddress::parseSubnet(subnet);
            if (parsed.first.isNull() || parsed.second < 0) {
                return std::nullopt;
            }
            QString &address = parsed.first.protocol() == QAbstractSocket::IPv6Protocol ? result.ipv6 : result.ipv4;
            if (!address.isNull()) {
                return std::nullopt;
            }
            address = subnet;
        }
        return result;
    }

    inline QString formatEndpoint(const QString &address, int port)
    {
        const QHostAddress parsed(address);
        if (parsed.protocol() == QAbstractSocket::IPv6Protocol) {
            return QStringLiteral("[%1]:%2").arg(parsed.toString()).arg(port);
        }
        return QStringLiteral("%1:%2").arg(address).arg(port);
    }
}
