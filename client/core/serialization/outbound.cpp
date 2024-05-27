// Copyright (c) Qv2ray, A Qt frontend for V2Ray. Written in C++.
// This file is part of the Qv2ray VPN client.
//
// Qv2ray, A Qt frontend for V2Ray. Written in C++

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Copyright (c) 2024 AmneziaVPN
// This file has been modified for AmneziaVPN
//
// This file is based on the work of the Qv2ray VPN client.
// The original code of the Qv2ray, A Qt frontend for V2Ray. Written in C++ and licensed under GPL3.
//
// The modified version of this file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file. If not, see <https://www.gnu.org/licenses/>.

#include <QString>
#include <QJsonObject>
#include <QList>
#include "3rd/QJsonStruct/QJsonIO.hpp"
#include "transfer.h"
#include "serialization.h"

namespace amnezia::serialization::outbounds
{
QJsonObject GenerateFreedomOUT(const QString &domainStrategy, const QString &redirect)
{
    QJsonObject root;
    JADD(domainStrategy, redirect)
    return root;
}

QJsonObject GenerateBlackHoleOUT(bool useHTTP)
{
    QJsonObject root;
    QJsonObject resp;
    resp.insert("type", useHTTP ? "http" : "none");
    root.insert("response", resp);
    return root;
}

QJsonObject GenerateShadowSocksServerOUT(const QString &address, int port, const QString &method, const QString &password)
{
    QJsonObject root;
    JADD(address, port, method, password)
    return root;
}

QJsonObject GenerateShadowSocksOUT(const QList<ShadowSocksServerObject> &_servers)
{
    QJsonObject root;
    QJsonArray x;

    for (const auto &server : _servers)
    {
        x.append(GenerateShadowSocksServerOUT(server.address, server.port, server.method, server.password));
    }

    root.insert("servers", x);
    return root;
}

QJsonObject GenerateHTTPSOCKSOut(const QString &addr, int port, bool useAuth, const QString &username, const QString &password)
{
    QJsonObject root;
    QJsonIO::SetValue(root, addr, "servers", 0, "address");
    QJsonIO::SetValue(root, port, "servers", 0, "port");
    if (useAuth)
    {
        QJsonIO::SetValue(root, username, "servers", 0, "users", 0, "user");
        QJsonIO::SetValue(root, password, "servers", 0, "users", 0, "pass");
    }
    return root;
}

QJsonObject GenerateOutboundEntry(const QString &tag, const QString &protocol, const QJsonObject &settings, const QJsonObject &streamSettings,
                                  const QJsonObject &mux, const QString &sendThrough)
{
    QJsonObject root;
    JADD(sendThrough, protocol, settings, tag, streamSettings, mux)
    return root;
}

QJsonObject GenerateTrojanOUT(const QList<TrojanObject> &_servers)
{
    QJsonObject root;
    QJsonArray x;

    for (const auto &server : _servers)
    {
        x.append(GenerateTrojanServerOUT(server.address, server.port, server.password));
    }

    root.insert("servers", x);
    return root;
}

QJsonObject GenerateTrojanServerOUT(const QString &address, int port, const QString &password)
{
    QJsonObject root;
    JADD(address, port, password)
    return root;
}

} // namespace amnezia::serialization::outbounds

