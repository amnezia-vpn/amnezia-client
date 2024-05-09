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


#include "3rd/QJsonStruct/QJsonIO.hpp"
#include <QUrlQuery>
#include "serialization.h"

#define OUTBOUND_TAG_PROXY "PROXY"

namespace amnezia::serialization::trojan
{
QJsonObject Deserialize(const QString &trojanUri, QString *alias, QString *errMessage)
{
    // must start with trojan://
    if (!trojanUri.startsWith("trojan://"))
    {
        *errMessage = QObject::tr("Trojan link should start with trojan://");
        return QJsonObject();
    }

    TrojanServerObject server;
    QString d_name;

    if (trojanUri.length() < 9)
    {
        *errMessage = QObject::tr("SS URI is too short");
    }

    auto url = QUrl::fromUserInput(trojanUri);
    if (url.scheme() != "trojan")
    {
        *errMessage = QObject::tr("Not a Trojan share link!");
    }
    server.address = url.host();
    server.port = url.port();
    server.password = url.userInfo();
    if (url.hasFragment())
    {
        d_name = url.fragment(QUrl::FullyDecoded);
    }

    QJsonObject root;
    QJsonArray outbounds;
    outbounds.append(outbounds::GenerateOutboundEntry(OUTBOUND_TAG_PROXY, "trojan", outbounds::GenerateTrojanOUT({ server }), {}));
    JADD(outbounds)
    *alias = alias->isEmpty() ? d_name : *alias + "_" + d_name;
    return root;
}

const QString Serialize(const TrojanServerObject &server, const QString &alias)
{
    QUrl url;
    url.setUserInfo(server.password);
    url.setScheme("trojan");
    url.setHost(server.address);
    url.setPort(server.port);
    url.setFragment(alias);
    return url.toString(QUrl::ComponentFormattingOption::FullyEncoded);
}
} // namespace amnezia::serialization::trojan

